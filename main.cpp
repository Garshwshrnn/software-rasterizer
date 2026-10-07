#include <iostream>
#include <cmath>
#include <random> 
#include <cstdint>
#include <algorithm>

#include <vector>
#include <fstream>
#include <string>
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

// MATH FUNCS
float Dot(float2 a, float2 b){ return a.x * b.x + a.y * b.y ;}
float2 Perpendicular(float2 vec) { return float2(vec.y, -vec.x);}
bool PointOnRightSideOfLine(float2 a, float2 b, float2 p){
    float2 ap = p + -a;
    float2 abPerp = Perpendicular(b + -a);
    return Dot(ap, abPerp) >= 0;
}
bool PointInTriangle(float2 a, float2 b, float2 c, float2 p) {
    bool sideAB = PointOnRightSideOfLine(a,b,p);
    bool sideBC = PointOnRightSideOfLine(b,c,p);
    bool sideCA = PointOnRightSideOfLine(c,a,p);//make sure not AC
    return sideAB == sideBC && sideBC == sideCA;
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

// IMG TO FILE
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


const int WIDTH = 600, HEIGHT = 400;
bool isRasterizing = true;
int frameCount = 0;

// RENDER
void drawTriangle(
    Framebuffer& fb,
    float2 a,
    float2 b,
    float2 c,
    uint32_t color)
{
    int minX = static_cast<int>(
        std::min({a.x, b.x, c.x})
    );

    int maxX = static_cast<int>(
        std::max({a.x, b.x, c.x})
    );

    int minY = static_cast<int>(
        std::min({a.y, b.y, c.y})
    );

    int maxY = static_cast<int>(
        std::max({a.y, b.y, c.y})
    );

    minX = std::max(minX, 0);
    minY = std::max(minY, 0);

    maxX = std::min(maxX, fb.width - 1);
    maxY = std::min(maxY, fb.height - 1);

    float area =
        (b.x - a.x) * (c.y - a.y) -
        (b.y - a.y) * (c.x - a.x);

    if (area == 0.0f)
        return;

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            float px = x + 0.5f;
            float py = y + 0.5f;

            float w0 =
                (b.x - a.x) * (py - a.y) -
                (b.y - a.y) * (px - a.x);

            float w1 =
                (c.x - b.x) * (py - b.y) -
                (c.y - b.y) * (px - b.x);

            float w2 =
                (a.x - c.x) * (py - c.y) -
                (a.y - c.y) * (px - c.x);

            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (w0 <= 0 && w1 <= 0 && w2 <= 0))
            {
                fb.setPixel(x, y, color);
            }
        }
    }
}
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

void Render(SDL_Renderer* renderer, Framebuffer framebuffer, SDL_Texture* texture, vector<float2>& points, vector<float2>& velocities, vector<float3>& triangleColors){
    // Clear framebuffer
    framebuffer.clear(
        0xFF202020 //opaque very dark gray default color (AARRGGBB)
    );

    // stuff to render goes here
    for(size_t i=0; i<points.size(); i+=3){
        float2 a(points[i+0]);
        float2 b(points[i+1]);
        float2 c(points[i+2]);
        //bounding boxes; limit the possible points that the thing is in so that rendering is more efficient
        float minx = min(min(a.x, b.x), c.x);
        float miny = min(min(a.y, b.y), c.y);
        float maxx = max(max(a.x, b.x), c.x);
        float maxy = max(max(a.y, b.y), c.y);
        //AND make sure within bounding box of screen
        int blockStartx = max(min((int)minx, WIDTH), 0);
        int blockStarty = max(min((int)miny, HEIGHT), 0);
        int blockEndx = max(min((int)maxx, WIDTH), 0);
        int blockEndy = max(min((int)maxy, HEIGHT), 0);
        
        for(int y=blockStarty; y<blockEndy; y++){
            for(int x=blockStartx; x<blockEndx; x++){
                
                float2 p(x,y);

                if (PointInTriangle(a,b,c,p)){
                    framebuffer.setPixel(x,y,triangleColors[i/3].getARGB());
                }
            }
        }

        //update positions due to velocity
        for (int offset = 0; offset < 3; ++offset) {
            int idx = i + offset;
            auto res = MoveAndBouncePoint(points[idx], velocities[idx], 0,0,WIDTH, HEIGHT);
            points[idx]     = res[0];
            velocities[idx] = res[1];
        }
    }
    // end of stuff to render
    
    SDL_UpdateTexture(
        texture,
        nullptr,
        framebuffer.pixels.data(),
        WIDTH * sizeof(uint32_t)
    );
    
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}
void handleEvents(){
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {

        case SDL_QUIT: // handling of close button
            isRasterizing = false;
            break;

        case SDL_KEYDOWN:
            // keyboard API for key pressed
            /*
            switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_W:
            case SDL_SCANCODE_UP:
                dest.y -= speed / 30;
                break;
            case SDL_SCANCODE_A:
            case SDL_SCANCODE_LEFT:
                dest.x -= speed / 30;
                break;
            case SDL_SCANCODE_S:
            case SDL_SCANCODE_DOWN:
                dest.y += speed / 30;
                break;
            case SDL_SCANCODE_D:
            case SDL_SCANCODE_RIGHT:
                dest.x += speed / 30;
                break;
            default:
                break;
            }*/
           break;
        default:
            break;
        }
    }
}
void Run(vector<float2>& points, vector<float2>& velocities, vector<float3>& triangleColors){
    cout << "executing Run()" << endl;
    isRasterizing = true;
    bool isDone = true;
    int doneCounter = 0;

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

    cout << "isRasterizing: " << isRasterizing << ", isDone: " << isDone << endl;
    while(isRasterizing && isDone){
        cout << "Frame " << frameCount << " render time: " << GetFrameTimeMs() << "ms" << '\n';
        frameCount++;

        Render(renderer, framebuffer, texture, points, velocities, triangleColors);
        handleEvents();

        if(false){
            doneCounter++;
            if(doneCounter>10000) isDone = false; 
        }
    };

    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}



// testing
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
void CreateTestImages(){
    const int triangleCount = 40;

    //float2 points[triangleCount*3];
    vector<float2> points = {};
    vector<float2> velocities = {};
    vector<float3> triangleColors = {};

    float2 halfSize(WIDTH/2.0f, HEIGHT/2.0f);
    
        random_device rd;
        mt19937 gen(rd());
        /*uniform_real_distribution<double> distrib(0.0, 1.0);
    double rng = distrib(gen);*/

    //generate points
    for (int i=0; i<triangleCount*3; i++){
        points.push_back(halfSize + (RandomFloat2(gen, WIDTH, HEIGHT) - halfSize) * 0.5f);
    }
    //generate velocities and colors
    for (int i=0; i<triangleCount; i++){
        float2 velocity((RandomFloat2(gen, WIDTH/2, HEIGHT/2) - halfSize * 0.5) *0.05f);
        velocities.push_back(velocity);
        velocities.push_back(velocity);
        velocities.push_back(velocity);
        triangleColors.push_back(RandomColor(gen));
    }

    Run(points, velocities, triangleColors);
}

int main(int argc, char *argv[])
{
    cout << "Starting code!" << endl;
    CreateTestImages();
    return 0;
}
