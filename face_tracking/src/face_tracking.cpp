#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/objdetect.hpp>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>

using namespace cv;
using namespace std;

// Export macro for Windows DLL
#ifdef FACE_TRACKING_EXPORTS
#define FACE_TRACKING_API __declspec(dllexport)
#else
#define FACE_TRACKING_API __declspec(dllimport)
#endif

// Global variables
bool param_only_track_one_face = false;
static CascadeClassifier face_cascade;
static CascadeClassifier profile_cascade;
static bool classifiers_loaded = false;

// Struct to match Python's ProcessFrameResult
struct ProcessFrameResult
{
    float cx, cy, w, h;
    int orientation;
};

// Face struct for internal use
struct Face
{
    int cx, cy, w, h, orientation;
    Face(int cx_, int cy_, int w_, int h_, int orient_)
        : cx(cx_), cy(cy_), w(w_), h(h_), orientation(orient_) {}
};

// Filter close faces
vector<Face> filter_close_faces(vector<Face> &faces, int image_width, int min_dist_x)
{
    if (faces.empty())
        return {};

    // Sort by x-coordinate
    sort(faces.begin(), faces.end(), [](const Face &a, const Face &b)
         { return a.cx < b.cx; });

    vector<Face> filtered_faces = {faces[0]};
    for (size_t i = 1; i < faces.size(); ++i)
    {
        if (abs(faces[i].cx - filtered_faces.back().cx) >= min_dist_x)
        {
            filtered_faces.push_back(faces[i]);
        }
    }
    return filtered_faces;
}

// Simple Base64 decoding function
string base64_decode(const string &encoded)
{
    auto start = std::chrono::high_resolution_clock::now();
    static const string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    string decoded;
    int in_len = encoded.size();
    int i = 0, j = 0, in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];

    while (in_len-- && (encoded[in_] != '=') &&
           (isalnum(encoded[in_]) || encoded[in_] == '+' || encoded[in_] == '/'))
    {
        char_array_4[i++] = encoded[in_];
        in_++;
        if (i == 4)
        {
            for (i = 0; i < 4; i++)
            {
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            }
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
            {
                decoded += char_array_3[i];
            }
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;
        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; j < i - 1; j++)
            decoded += char_array_3[j];
    }
    auto end = std::chrono::high_resolution_clock::now();

    // Calculate the duration in different units
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Print the results
    std::cout << "Decoding time (milliseconds): " << duration_ms.count() << " ms\n"
              << std::endl;

    return decoded;
}

// Decode Base64 string to OpenCV Mat
Mat decode_base64_to_frame(const string &data)
{
    size_t comma_pos = data.find(',');
    if (comma_pos == string::npos || comma_pos + 1 >= data.size())
    {
        throw runtime_error("Invalid data format: no comma found or empty data");
    }
    string base64_data = data.substr(comma_pos + 1);
    string decoded_data = base64_decode(base64_data);
    auto start = std::chrono::high_resolution_clock::now();
    if (decoded_data.empty())
    {
        throw runtime_error("Base64 decoding resulted in empty data");
    }
    vector<uchar> buffer(decoded_data.begin(), decoded_data.end());
    Mat frame = imdecode(buffer, IMREAD_COLOR);
    if (frame.empty())
    {
        throw runtime_error("Failed to decode frame with cv::imdecode");
    }
    auto end = std::chrono::high_resolution_clock::now();

    // Calculate the duration in different units
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Print the results
    std::cout << "base64 to frame time (milliseconds): " << duration_ms.count() << " ms\n"
              << std::endl;
    return frame;
}

// Modified process_frame to return ProcessFrameResult
ProcessFrameResult process_frame(const string &encoded_frame,
                                 CascadeClassifier &face_cascade,
                                 CascadeClassifier &profile_cascade)
{
    // Decode the Base64 string into a Mat
    Mat frame;
    try
    {
        frame = decode_base64_to_frame(encoded_frame);
    }
    catch (const runtime_error &e)
    {
        cerr << "Error decoding frame: " << e.what() << endl;
        return {0.0f, 0.0f, 0.0f, 0.0f, 0};
    }
    auto start = std::chrono::high_resolution_clock::now();
    int h = frame.rows, w = frame.cols;
    const float scale_factor = 0.5f;
    Mat small_frame, gray, flipped_gray;

    // Downscale and convert to grayscale
    resize(frame, small_frame, Size(), scale_factor, scale_factor, INTER_LINEAR);
    cvtColor(small_frame, gray, COLOR_BGR2GRAY);
    if (!param_only_track_one_face)
    {
        flip(gray, flipped_gray, 1);
    }

    const float scale_back = 1.0f / scale_factor;
    vector<Face> detected_faces;
    Size min_size(15, 15);

    // Front face detection
    vector<Rect> faces;
    face_cascade.detectMultiScale(gray, faces, 1.25, 5, 0, min_size);
    for (const auto &r : faces)
    {
        int x = static_cast<int>(r.x * scale_back);
        int y = static_cast<int>(r.y * scale_back);
        int w_f = static_cast<int>(r.width * scale_back);
        int h_f = static_cast<int>(r.height * scale_back);
        detected_faces.emplace_back(x + w_f / 2, y + h_f / 2, w_f, h_f, 1);
    }

    if (!param_only_track_one_face)
    {
        // Left-profile detection
        vector<Rect> left_profiles;
        profile_cascade.detectMultiScale(gray, left_profiles, 1.25, 5, 0, min_size);
        for (const auto &r : left_profiles)
        {
            int x = static_cast<int>(r.x * scale_back);
            int y = static_cast<int>(r.y * scale_back);
            int w_p = static_cast<int>(r.width * scale_back);
            int h_p = static_cast<int>(r.height * scale_back);
            detected_faces.emplace_back(x + w_p / 2, y + h_p / 2, w_p, h_p, 3);
        }

        // Right-profile detection
        vector<Rect> right_profiles;
        profile_cascade.detectMultiScale(flipped_gray, right_profiles, 1.25, 5, 0, min_size);
        for (const auto &r : right_profiles)
        {
            int x = w - static_cast<int>((r.x + r.width) * scale_back);
            int y = static_cast<int>(r.y * scale_back);
            int w_p = static_cast<int>(r.width * scale_back);
            int h_p = static_cast<int>(r.height * scale_back);
            detected_faces.emplace_back(x + w_p / 2, y + h_p / 2, w_p, h_p, 2);
        }
    }

    // Filter close faces
    int min_dist_x = static_cast<int>(0.05 * w);
    vector<Face> filtered_faces = filter_close_faces(detected_faces, w, min_dist_x);

    if (filtered_faces.empty())
    {
        auto end = std::chrono::high_resolution_clock::now();

        // Calculate the duration in different units
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // Print the results
        std::cout << "processing time (milliseconds): " << duration_ms.count() << " ms\n";
        return {0.0f, 0.0f, 0.0f, 0.0f, 0};
    }

    if (filtered_faces.size() == 1 || param_only_track_one_face)
    {
        const Face &f = filtered_faces[0];
        auto end = std::chrono::high_resolution_clock::now();

        // Calculate the duration in different units
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // Print the results
        std::cout << "processing time (milliseconds): " << duration_ms.count() << " ms\n"
                  << std::endl;
        return {
            round((f.cx / static_cast<float>(w)) * 100.0f * 100.0f) / 100.0f,
            round((f.cy / static_cast<float>(h)) * 100.0f * 100.0f) / 100.0f,
            round((f.w / static_cast<float>(w)) * 100.0f * 100.0f) / 100.0f,
            round((f.h / static_cast<float>(h)) * 100.0f * 100.0f) / 100.0f,
            f.orientation};
    }

    // Average two largest faces
    sort(filtered_faces.begin(), filtered_faces.end(),
         [](const Face &a, const Face &b)
         { return a.w * a.h > b.w * b.h; });
    int x1 = filtered_faces[0].cx, y1 = filtered_faces[0].cy;
    int x2 = filtered_faces.size() > 1 ? filtered_faces[1].cx : x1;
    int y2 = filtered_faces.size() > 1 ? filtered_faces[1].cy : y1;
    float avg_x = (x1 + x2) / 2.0f, avg_y = (y1 + y2) / 2.0f;
    float dist_x = abs(x1 - x2), dist_y = abs(y1 - y2);
    auto end = std::chrono::high_resolution_clock::now();

    // Calculate the duration in different units
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Print the results
    std::cout << "processing time (milliseconds): " << duration_ms.count() << " ms\n"
              << std::endl;
    return {
        round((avg_x / static_cast<float>(w)) * 100.0f * 100.0f) / 100.0f,
        round((avg_y / static_cast<float>(h)) * 100.0f * 100.0f) / 100.0f,
        round((dist_x / static_cast<float>(w)) * 100.0f * 100.0f) / 100.0f,
        round((dist_y / static_cast<float>(h)) * 100.0f * 100.0f) / 100.0f,
        1};
}

// Exported functions for Python
extern "C"
{
    // Initialize classifiers
    FACE_TRACKING_API void init_classifiers(const char *face_cascade_path, const char *profile_cascade_path)
    {
        if (!classifiers_loaded)
        {
            if (!face_cascade.load(face_cascade_path) || !profile_cascade.load(profile_cascade_path))
            {
                cerr << "Error: Could not load cascade classifiers" << endl;
                exit(1);
            }
            classifiers_loaded = true;
        }
    }

    // Process a frame and return the result
    FACE_TRACKING_API ProcessFrameResult process_frame(const char *encoded_frame)
    {
        if (!classifiers_loaded)
        {
            cerr << "Error: Classifiers not initialized. Call init_classifiers first." << endl;
            return {0.0f, 0.0f, 0.0f, 0.0f, 0};
        }
        return process_frame(string(encoded_frame), face_cascade, profile_cascade);
    }
}