#include <iostream>
#include <cmath>
#include <random> 
#include <cstdint>
#include <algorithm>

#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <span>

#include <SDL.h>

using namespace std;

// STRUCTURES
    //math
struct float3 {
    float x, y, z;
    float r, g, b;
    
    float3(float x = 0, float y = 0, float z = 0) 
        : x(x), y(y), z(z), r(x), g(y), b(z) {}

    uint32_t getARGB(uint8_t alpha = 255) {
        uint32_t a     = static_cast<uint32_t>(alpha);
        uint32_t red   = static_cast<uint32_t>(r * 255.0f);
        uint32_t green = static_cast<uint32_t>(g * 255.0f);
        uint32_t blue  = static_cast<uint32_t>(b * 255.0f);

        return (a << 24) | (red << 16) | (green << 8) | blue;
    }

    float3 operator*(float scalar) const {
        return float3(x * scalar, y * scalar, z * scalar);
    }
    float3 operator+(float3 other) const {
        return float3(x + other.x, y + other.y, z + other.z);
    }
};
struct float2 {
    float x, y;
    
    float2(float x = 0, float y = 0) 
        : x(x), y(y) {}

    float2 operator+(const float2& other) const {
        return float2(this->x + other.x, this->y + other.y);
    }
    float2 operator-() const {
        return float2(-x, -y);
    }

    float2 operator-(const float2& other) const {
        return float2(x - other.x, y - other.y);
    }

    float2 operator*(float scalar) const {
        return float2(x * scalar, y * scalar);
    }
};
    //render
struct Framebuffer{
    int width;
    int height;

    vector<uint32_t> pixels;

    Framebuffer(int w, int h)
        : width(w),
          height(h),
          pixels(w * h)
    {
    }

    void clear(uint32_t color)
    {
        fill(pixels.begin(), pixels.end(), color);
    }

    void setPixel(int x, int y, uint32_t color)
    {
        if (x < 0 || x >= width ||
            y < 0 || y >= height)
            return;

        pixels[y * width + x] = color;
    }
};
struct Model{
    vector<float3> trianglePoints;
    vector<float3> triangleColors;
    float3 velocity;

    Model(vector<float3> tp, vector<float3> tc, float3 v)
        : trianglePoints(tp),
          triangleColors(tc),
          velocity(v)
    {
    };
};

// MATH FUNCS
float Dot(float2 a, float2 b){ return a.x * b.x + a.y * b.y ;}
float Dot(float ax, float ay, float bx, float by){ return ax * bx + ay * by ;}
float2 Perpendicular(float2 vec) { return float2(vec.y, -vec.x);}//clockwise
bool PointOnRightSideOfLine(float2 a, float2 b, float2 p){
    float2 ap = p + -a;
    float2 abPerp = Perpendicular(b + -a);
    return Dot(ap, abPerp) >= 0;
}
bool PointOnRightSideOfLine(float ax, float ay, float bx, float by, float px, float py){
    float apx = px - ax;
    float apy = py - ay;
    //simplified perpendicular
    float abPerpx = by-ay;
    float abPerpy = -bx+ax;
    return Dot(apx, apy, abPerpx, abPerpy) >= 0;
}
/*bool PointInTriangle(float2 a, float2 b, float2 c, float2 p) {
    bool sideAB = PointOnRightSideOfLine(a,b,p);
    bool sideBC = PointOnRightSideOfLine(b,c,p);
    bool sideCA = PointOnRightSideOfLine(c,a,p);//make sure not AC
    return sideAB && sideBC && sideCA;//backface culling
}*/
    //this float only version is like 4 times faster :sob:
bool PointInTriangle(float ax, float ay, float bx, float by, float cx, float cy, float px, float py) {
    bool sideAB = PointOnRightSideOfLine(ax,ay, bx,by, px,py);
    bool sideBC = PointOnRightSideOfLine(bx,by, cx,cy, px,py);
    bool sideCA = PointOnRightSideOfLine(cx,cy, ax,ay, px,py);//make sure not AC
    return sideAB && sideBC && sideCA;//backface culling
}
    //string helpers
vector<string> SplitByLine(const string& str) { 
    vector<string> lines; 
    stringstream ss(str); 
    string line; 
    while (getline(ss, line)) { 
        // Remove trailing carriage return '\r' if it's a Windows newline (\r\n) 
        if (!line.empty() && line.back() == '\r') { 
            line.pop_back(); 
        } 
        lines.push_back(line); 
    } 
    return lines; 
} 
std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = s.find(delimiter);

    while (end != std::string::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + 1;
        end = s.find(delimiter, start);
    }
    
    // Add the remaining last token
    tokens.push_back(s.substr(start));

    return tokens;
}
    //random helpers
float2 RandomFloat2(std::mt19937& gen, float xmax, float ymax){
    std::uniform_real_distribution<float> xDist(0.0f, xmax);
    std::uniform_real_distribution<float> yDist(0.0f, ymax);

    return float2(xDist(gen), yDist(gen));
}
float3 RandomColor(std::mt19937& gen) {
    // Defines a uniform distribution between 0.0 and 1.0
    std::uniform_real_distribution<float> distrib(0.0f, 1.0f);
    
    // Returns a float3 packed with random Red, Green, and Blue values
    return float3(distrib(gen), distrib(gen), distrib(gen));
}
float3 RandomColor() {
    random_device rd; mt19937 gen(rd());
    // Defines a uniform distribution between 0.0 and 1.0
    std::uniform_real_distribution<float> distrib(0.0f, 1.0f);
    
    // Returns a float3 packed with random Red, Green, and Blue values
    return float3(distrib(gen), distrib(gen), distrib(gen));
}
vector<float3> GenerateRandomTriangleColors(int numberOfColors){
    vector<float3> list = {};
    random_device rd; mt19937 gen(rd());
    
    for(int i=0; i<numberOfColors; i++) { list.push_back(RandomColor( gen));}

    return list;
}

//TIME MANAGEMENT
#include <chrono>
auto startFrameTime = std::chrono::high_resolution_clock::now();
auto endFrameTime = std::chrono::high_resolution_clock::now();
double GetFrameTimeMs(){
    startFrameTime = endFrameTime;
    endFrameTime = std::chrono::high_resolution_clock::now();
    double frameTimeMs =
        std::chrono::duration<double, std::milli>(
            endFrameTime - startFrameTime
        ).count();
    return frameTimeMs;
}
using Clock = std::chrono::steady_clock;

static const auto start_time = Clock::now();
static auto pause_start = Clock::now();

float paused_seconds = 0.0f;

// FILE MANAGEMENT
void WriteImageToFile(const vector<vector<float3>> image, const string filenameWithType){
    // Open file in binary mode
    std::ofstream writer(filenameWithType, std::ios::binary);
    if (!writer) {
        std::cerr << "Failed to open file: " << filenameWithType << std::endl;
        return;
    }

    int width = image.size();
    int height = image[0].size();
    uint32_t dataSize = width * height * 4;
    uint32_t totalSize = 14 + 40 + dataSize;

    // BMP Header (14 bytes)
    writer.write("BM", 2);                         // Signature
    writer.write(reinterpret_cast<char*>(&totalSize), 4); // File size
    uint32_t reserved = 0;
    writer.write(reinterpret_cast<char*>(&reserved), 4);  // Reserved
    uint32_t offset = 14 + 40;
    writer.write(reinterpret_cast<char*>(&offset), 4);    // Data offset

    // DIB Header (40 bytes)
    uint32_t dibSize = 40;
    writer.write(reinterpret_cast<char*>(&dibSize), 4);   // Header size
    writer.write(reinterpret_cast<char*>(&width), 4);     // Width
    writer.write(reinterpret_cast<char*>(&height), 4);    // Height
    uint16_t planes = 1;
    writer.write(reinterpret_cast<char*>(&planes), 2);    // Color planes
    uint16_t bpp = 32;
    writer.write(reinterpret_cast<char*>(&bpp), 2);       // Bits per pixel (32-bit RGBA)
    uint32_t compression = 0;
    writer.write(reinterpret_cast<char*>(&compression), 4);// No compression
    writer.write(reinterpret_cast<char*>(&dataSize), 4);   // Image data size
    
    // Remaining DIB fields (16 bytes total of zeros)
    char zero[16] = {0};
    writer.write(zero, 16);

    // Pixel Data (BGRA format for standard BMP files)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float3 col = image[x][y];
            
            // BMP pixels are typically stored in BGRA order
            uint8_t b = static_cast<uint8_t>(std::min(max(col.b * 255.0f, 0.0f), 255.0f));
            uint8_t g = static_cast<uint8_t>(std::min(max(col.g * 255.0f, 0.0f), 255.0f));
            uint8_t r = static_cast<uint8_t>(std::min(max(col.r * 255.0f, 0.0f), 255.0f));
            uint8_t a = 0; // Padding / Alpha

            writer.write(reinterpret_cast<char*>(&b), 1);
            writer.write(reinterpret_cast<char*>(&g), 1);
            writer.write(reinterpret_cast<char*>(&r), 1);
            writer.write(reinterpret_cast<char*>(&a), 1);
        }
    }    
}
    // Highly inefficient and incomplete obj parser
vector<float3> LoadObjFile(string objString) {
    vector<float3> allPoints = {};
    vector<float3> trianglePoints = {}; // each set of 3 points is a triangle

    for(const std::string& line : SplitByLine(objString))
    {
        if (line.rfind("v ", 0) == 0) // vertex positions
        {
            vector<string> axes = split(line.substr(2),' ');
            allPoints.push_back(float3(stof(axes[0]), stof(axes[1]), stof(axes[2])));
        }
        else if (line.rfind("f ", 0) == 0) // face indices
        {
            vector<string> faceIndexGroups = split(line.substr(2),' ');
            
            int tpsize = faceIndexGroups.size();
            float3 firstVer, lastVer;
            for (int i = 0; i < tpsize; i++)
            {
                vector<string> indexGroup = split(faceIndexGroups[i],'/');
                int pointIndex = stoi(indexGroup[0]) - 1; // subtract one since indices start at 1 in obj
                if(tpsize>3){// n-gon triangle fan
                    if(i==0) firstVer = allPoints[pointIndex]; //readd first vertex of prev triangle
                    if(i==2) lastVer  = allPoints[pointIndex]; //readd last vertex of prev triangle
                    if(i>2){
                        trianglePoints.push_back(firstVer);
                        trianglePoints.push_back(lastVer);
                        if(i > 3) lastVer = allPoints[pointIndex];
                    }
                } 
                trianglePoints.push_back(allPoints[pointIndex]);
            }
        }
    }

    return trianglePoints;
}


const int WIDTH = 600, HEIGHT = 400;
const float FOCAL_L = 0.5f;
bool isRasterizing = true;
bool isPaused = false, stepRequested = false;
int frameCount = 0; float frameStepDuration = 0.1f;

// RENDER
    //helpers
vector<float2> MoveAndBouncePoint(float2 initial, float2 velocity, float minx, float miny, float maxx, float maxy) {
    float2 result = initial + velocity;
    float2 resultVel = velocity;

    // Check and handle X-axis boundaries
    if (result.x > maxx) {
        result.x = maxx - (result.x - maxx); // Reflect position inside the box
        resultVel.x = -velocity.x;           // Reverse velocity
    } else if (result.x < minx) {
        result.x = minx + (minx - result.x); // Reflect position inside the box
        resultVel.x = -velocity.x;           // Reverse velocity
    }

    // Check and handle Y-axis boundaries
    if (result.y > maxy) {
        result.y = maxy - (result.y - maxy); // Reflect position inside the box
        resultVel.y = -velocity.y;           // Reverse velocity
    } else if (result.y < miny) {
        result.y = miny + (miny - result.y); // Reflect position inside the box
        resultVel.y = -velocity.y;           // Reverse velocity
    }

    vector<float2> res = {result, resultVel};
    return res;
}
void drawTriangle(Framebuffer& fb, float2 a, float2 b, float2 c, uint32_t color) {
    int minX = static_cast<int>(std::min({a.x, b.x, c.x}));
    int maxX = static_cast<int>(std::max({a.x, b.x, c.x}));
    int minY = static_cast<int>(std::min({a.y, b.y, c.y}));
    int maxY = static_cast<int>(std::max({a.y, b.y, c.y}));

    minX = std::max(minX, 0);
    minY = std::max(minY, 0);

    maxX = std::min(maxX, fb.width - 1);
    maxY = std::min(maxY, fb.height - 1);

    float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);

    if (area == 0.0f) return;

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            float px = x + 0.5f;
            float py = y + 0.5f;

            //if(PointInTriangle(a, b, c, float2(px,py))) fb.setPixel(x, y, color);
            if(PointInTriangle(a.x,a.y, b.x,b.y, c.x,c.y, px,py)) fb.setPixel(x, y, color);
        }
    }
}

class Transform{
public:
    float Yaw; // Rotation around y axis
    float Pitch; // Rotation around  axis
    float3 Position;

    Transform(float y=0.0f, float p=0.0f, float3 pos=float3(0,0,0)) {
      Yaw = y;
      Pitch = p;
      Position = pos;
    }

    float3 ToWorldPoint(float3 pt)
    {
        return TransformVector(GetBasisVectors(1), GetBasisVectors(2), GetBasisVectors(3), pt) + Position;
    }

private:
    // Calculate right/up/forward vectors (i, ĵ, k)
    float3 GetBasisVectors(int index)
    {
        float3 ihat_yaw = float3(cos(Yaw), 0, sin(Yaw));
        float3 jhat_yaw = float3(0, 1, 0);
        float3 khat_yaw = float3(-1*sin(Yaw), 0, cos(Yaw));

        float3 vec;
        if(index == 1){//i
            float3 ihat_pitch(1,0,0);
            vec = TransformVector(ihat_yaw,jhat_yaw,khat_yaw, ihat_pitch);
        }
        if(index == 2){//j
            float3 jhat_pitch(0, cos(Pitch), -1*sin(Pitch));
            vec = TransformVector(ihat_yaw,jhat_yaw,khat_yaw, jhat_pitch);
        } 
        if(index == 3){//k
            float3 khat_pitch(0, 1*sin(Pitch), cos(Pitch));
            vec = TransformVector(ihat_yaw,jhat_yaw,khat_yaw, khat_pitch);
        } 
        return vec;
    }

    // Move each coordinate of given vector along the corresponding basis vector
    float3 TransformVector(float3 ihat, float3 jhat, float3 khat, float3 v)
    {
        return ihat * v.x + jhat * v.y + khat * v.z ;
    }
};
float3 WorldToScreen(const float3& point, float focalLength, Transform transform, float width, float height) {
    float pixelsPerWorldUnit = 70;
    
    float3 worldPoint = transform.ToWorldPoint(point);
    // If the point is behind or too close to the camera, return an invalid sentinel point
    //if (abs(worldPoint.z) <= 0.1f) return { float3((width / 2.0f), (height / 2.0f), 0.0f) }; 

    float projectedX = (worldPoint.x * pixelsPerWorldUnit * focalLength) / worldPoint.z;
    float projectedY = (worldPoint.y * pixelsPerWorldUnit * focalLength) / worldPoint.z;

    float3 outScreen;
    outScreen.x = (width / 2.0f) + projectedX;
    outScreen.y = (height / 2.0f) + projectedY; // Invert Y? because screen Y goes down
    outScreen.z = worldPoint.z;                 // Keep original depth for Z-buffering

    return outScreen;
}

void PreRender(Framebuffer& framebuffer){
    // Clear framebuffer
    framebuffer.clear(
        0x00202020 //opaque very dark gray default color (AARRGGBB)
    );
}
void PostRender(SDL_Renderer* renderer, Framebuffer& framebuffer, SDL_Texture* texture){
    SDL_UpdateTexture(texture, nullptr, framebuffer.pixels.data(), framebuffer.width * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}
void PresentFramebuffer(SDL_Renderer* renderer, Framebuffer& framebuffer, SDL_Texture* texture){
    SDL_UpdateTexture(
        texture,
        nullptr,
        framebuffer.pixels.data(),
        framebuffer.width * sizeof(uint32_t)
    );

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}
void Render(SDL_Renderer* renderer, Framebuffer& framebuffer, SDL_Texture* texture, vector<float3>& points, vector<float2>& velocities, vector<float3>& triangleColors){
    PreRender(framebuffer);

    float rotation_speed = 1.0f;
    auto now = Clock::now();
    float elapsed_seconds = std::chrono::duration<float>(
        now - start_time
    ).count();
    float current_paused_seconds = paused_seconds;
    float yaw = rotation_speed * (elapsed_seconds - current_paused_seconds);
    float pitch = 0.5f;
    Transform transformation(pitch, yaw, float3(0,0,2));


    // stuff to render goes here
    for(size_t i=0; i<points.size(); i+=3){

        float3 pixA = WorldToScreen(points[i+0], FOCAL_L, transformation, WIDTH, HEIGHT);
        float3 pixB = WorldToScreen(points[i+1], FOCAL_L, transformation, WIDTH, HEIGHT);
        float3 pixC = WorldToScreen(points[i+2], FOCAL_L, transformation, WIDTH, HEIGHT);
        drawTriangle(framebuffer, 
            float2(pixA.x,pixA.y),
            float2(pixB.x,pixB.y),
            float2(pixC.x,pixC.y),
            triangleColors[i/3].getARGB());

        //update positions due to velocity
        if(true) continue;
        for (int offset = 0; offset < 3; ++offset) {
            int idx = i + offset;
            auto res = MoveAndBouncePoint(float2(points[idx].x,points[idx].y), velocities[idx], 0,0, WIDTH,HEIGHT);
            points[idx] = float3(res[0].x,res[0].y,0);
            velocities[idx] = res[1];
        }
    }
    // end of stuff to render
    
    PresentFramebuffer(renderer, framebuffer, texture);
    //PostRender(renderer, framebuffer, texture);
}
void handleEvents(SDL_Renderer* renderer, Framebuffer& framebuffer, SDL_Texture* texture, vector<float3>& points, vector<float2>& velocities, vector<float3>& triangleColors){
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {

        case SDL_QUIT: // handling of close button
            isRasterizing = false;
            break;

        case SDL_KEYDOWN:
            // keyboard API for key pressed
            switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_SPACE:
                isPaused = !isPaused; // Toggle pause state
                if(isPaused){
                    cout << "⏸️ Paused" << endl;
                    pause_start = Clock::now();
                }
                else{
                    cout << "▶️ Resumed" << endl;
                    paused_seconds += std::chrono::duration<float>(
                        Clock::now() - pause_start
                    ).count();
                }
                break;
                
            case SDL_SCANCODE_F:
                if (isPaused) {
                    // Temporarily unpause for exactly one iteration
                    cout << "🎞️ Stepping forward 1 frame..." << endl;

                    paused_seconds += std::chrono::duration<float>(
                        Clock::now() - pause_start
                    ).count() - frameStepDuration;
                    pause_start = Clock::now();
                    
                    stepRequested = true;
                }
                break;
                
            case SDL_SCANCODE_R:
                if (isPaused) {
                    // Temporarily unpause for exactly one iteration
                    cout << "🎞️ Stepping backward 1 frame..." << endl;

                    paused_seconds += std::chrono::duration<float>(
                        Clock::now() - pause_start
                    ).count() + frameStepDuration;
                    pause_start = Clock::now();
                    
                    stepRequested = true;
                }
                break;
                
            case SDL_SCANCODE_L:
            {
                cout << "[L] draw triangle\n";
                PreRender(framebuffer);

                uint32_t color = RandomColor().getARGB();
        
                drawTriangle(framebuffer, 
                        float2(10.0f,10.0f),float2(10.0f,20.0f),float2(20.0f,10.0f), 
                        color);
                
                float2 a = float2(10.0f,10.0f);
                float2 b = float2(10.0f,20.0f);
                float2 c = float2(20.0f,10.0f);

                int minX = static_cast<int>(std::min({a.x, b.x, c.x}));
                int maxX = static_cast<int>(std::max({a.x, b.x, c.x}));
                int minY = static_cast<int>(std::min({a.y, b.y, c.y}));
                int maxY = static_cast<int>(std::max({a.y, b.y, c.y}));

                minX = std::max(minX, 0);
                minY = std::max(minY, 0);

                maxX = std::min(maxX, framebuffer.width - 1);
                maxY = std::min(maxY, framebuffer.height - 1);

                cout << "min: (" << minX << "," << minY << ") max: (" << maxX << "," << maxY << ")\n";

                float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);

                if (area == 0.0f) {return; cout<<"exit; area = 0\n";}

                for (int y = minY; y <= maxY; ++y) {
                    for (int x = minX; x <= maxX; ++x) {
                        float px = x + 0.5f;
                        float py = y + 0.5f;

                        if(PointInTriangle(a.x,a.y, b.x,b.y, c.x,c.y, px,py)){
                            cout << (y-minY)*(maxX-minX) + (x-minX) << "#("<<x<<","<<y<<") setting pixels\n";
                            framebuffer.setPixel(x, y, color);
                        }
                    }
                }
                PresentFramebuffer(renderer,framebuffer,texture);
            }
                break;

            default:
                break;
            }
           break;
        default:
            break;
        }
    }
}

void Run(vector<float3>& points, vector<float3>& triangleColors, vector<float2>& velocities){
    cout << "executing Run()" << endl;
    isRasterizing = true;
    bool isDone = true;
    int doneCounter = 0;

    if (velocities.empty()) {
        velocities.assign(points.size(), float2{0.0f, 0.0f});
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("error initializing SDL: %s\n", SDL_GetError());
    }
    int flags = 0;
    if(false/*make this if fullscreen*/){
        flags = SDL_WINDOW_FULLSCREEN;
    }
    SDL_Window* window = SDL_CreateWindow("C++ Software Rasterizer?",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT, flags);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0/*SDL_RENDERER_ACCELERATED?*/);
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,/* pixel format is 8 bytes per RGBA (AARRGGBB) */
        SDL_TEXTUREACCESS_STREAMING,/*continuous updates?*/
        WIDTH, HEIGHT);

    Framebuffer framebuffer(WIDTH, HEIGHT);

    
    if(1==0){
        uint32_t color = RandomColor().getARGB();
        
        drawTriangle(framebuffer, 
                float2(10.0f,10.0f),float2(10.0f,20.0f),float2(20.0f,10.0f), 
                color);
        
        float2 a = float2(10.0f,10.0f);
        float2 b = float2(10.0f,20.0f);
        float2 c = float2(20.0f,10.0f);

        int minX = static_cast<int>(std::min({a.x, b.x, c.x}));
        int maxX = static_cast<int>(std::max({a.x, b.x, c.x}));
        int minY = static_cast<int>(std::min({a.y, b.y, c.y}));
        int maxY = static_cast<int>(std::max({a.y, b.y, c.y}));

        minX = std::max(minX, 0);
        minY = std::max(minY, 0);

        maxX = std::min(maxX, framebuffer.width - 1);
        maxY = std::min(maxY, framebuffer.height - 1);

        float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);

        if (area == 0.0f) {return; cout<<"exit; area = 0\n";}

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                float px = x + 0.5f;
                float py = y + 0.5f;

                if(PointInTriangle(a.x,a.y, b.x,b.y, c.x,c.y, px,py)) framebuffer.setPixel(x, y, color);
            }
        }
    }

    cout << "isRasterizing: " << isRasterizing << ", isDone: " << isDone << ", isPaused: " << isPaused << endl;
    while(isRasterizing && isDone){
        
        if(!isPaused || stepRequested){
            cout << "Frame " << frameCount << " render time: " << GetFrameTimeMs() << "ms" << '\n';
            Render(renderer, framebuffer, texture, points, velocities, triangleColors);
            
            frameCount++;
            stepRequested = false;
        }
        handleEvents(renderer, framebuffer, texture, points, velocities, triangleColors);

        if(false){
            doneCounter++;
            if(doneCounter>10000) isDone = false; 
        }
    };

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
void Run(Model model){
    vector<float3> points = model.trianglePoints;
    vector<float3> triangleColors = model.triangleColors;
    vector<float2> velocities;
    velocities.assign(points.size(), float2(model.velocity.x,model.velocity.y));

    Run(points, triangleColors, velocities);
}

int main(int argc, char *argv[])
{
    cout << "Starting code!" << endl;
    
    string cube;
    if(1==1){
        cube = R"(
o Cube
v 1.000000 1.000000 -1.000000
v 1.000000 -1.000000 -1.000000
v 1.000000 1.000000 1.000000
v 1.000000 -1.000000 1.000000
v -1.000000 1.000000 -1.000000
v -1.000000 -1.000000 -1.000000
v -1.000000 1.000000 1.000000
v -1.000000 -1.000000 1.000000
vn -0.0000 1.0000 -0.0000
vn -0.0000 -0.0000 1.0000
vn -1.0000 -0.0000 -0.0000
vn -0.0000 -1.0000 -0.0000
vn 1.0000 -0.0000 -0.0000
vn -0.0000 -0.0000 -1.0000
vt 0.625000 0.500000
vt 0.875000 0.500000
vt 0.875000 0.750000
vt 0.625000 0.750000
vt 0.375000 0.750000
vt 0.625000 1.000000
vt 0.375000 1.000000
vt 0.375000 0.000000
vt 0.625000 0.000000
vt 0.625000 0.250000
vt 0.375000 0.250000
vt 0.125000 0.500000
vt 0.375000 0.500000
vt 0.125000 0.750000
s 0
f 1/1/1 5/2/1 7/3/1 3/4/1
f 4/5/2 3/4/2 7/6/2 8/7/2
f 8/8/3 7/9/3 5/10/3 6/11/3
f 6/12/4 2/13/4 4/5/4 8/14/4
f 2/13/5 1/1/5 3/4/5 4/5/5
f 6/11/6 5/10/6 1/1/6 2/13/6
        )";
    }
    
    vector<float3> trianglePoints = LoadObjFile(cube);
    vector<float3> randColors = GenerateRandomTriangleColors(trianglePoints.size()/3);

    Model cubeModel = Model(trianglePoints, randColors, float3(0.0f,0.0f,0.0f));
    Run(cubeModel);

    return 0;
}
