#include <iostream>
#include <cmath>
#include <chrono>

#include <vector>
#include <algorithm>
#include <fstream>
#include <string>

#include <SDL.h>

using namespace std;

// STRUCTURES
struct float3 {
    float x, y, z;
    float r, g, b;
    
    float3(float x = 0, float y = 0, float z = 0) 
        : x(x), y(y), z(z), r(x), g(y), b(z) {}
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
    
    
    /*using BinaryWriter writer = new(File.Open(GetFilePath(filenameWithType), FileMode.Create));
    uint[] ByteCounts = { 14, 40, (uint)image.Length * 4 }//BMP header, DIP header, data

    // HEADERS
    writer.Write("BM"u8.ToArray()); //bmp header start
    writer.Write(ByteCounts[0] + ByteCounts[1] + ByteCounts[2]); //total file size
    writer.Write((uint)0);
    writer.Write(ByteCounts[0] + ByteCounts[1]); //data offset from start
    writer.Write(ByteCounts[1]);// DIP header size
    writer.Write((uint)image.GetLength(0));//img width
    writer.Write((uint)image.GetLength(1));//img hegiht
    writer.Write((ushort)1); //num of color planes (?)
    writer.Write((ushort)(8*4)); //bits per pixel (1 byte per channel, 1 more for alignment)
    writer.Write((uint)0); //RGB FORMAT no compression
    writer.Write(ByteCounts[2]);//data size
    writer.Write(new byte[16]);

    // DATA
    for (int y = 0; y < image.GetLength(1); y++){
        for (int y = 0; y < image.GetLength(0); y++){
            float3 col = image[x,y];
            writer.Write((byte)(col.r * 255));
            writer.Write((byte)(col.g * 255));
            writer.Write((byte)(col.b * 255));
            writer.Write((byte)0);//padding
        }
    }*/
}

const int WIDTH = 600, HEIGHT = 400;
bool isRasterizing = true;
int frameCount = 0;

// RENDER
/*void drawLine(int x0, int y0, int x1, int y1, Color color, std::vector<uint32_t>& buffer) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT) {
            buffer[y0 * WIDTH + x0] = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}


int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

    SDL_Window* window = SDL_CreateWindow("C++ Software Rasterizer", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    vector<uint32_t> frameBuffer(WIDTH * HEIGHT, 0xFF000000);

    // 1. Define 8 local space vertices of a 1x1x1 Cube centered at (0,0,0)
    std::vector<float3> cubeVertices = {
        {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}
    };

    // 2. Define the 12 connecting edges of the cube faces
    std::vector<std::pair<int, int>> cubeEdges = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Front Face Edges
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Back Face Edges
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connecting Edges
    };

    Color boxColor{100, 200, 255, 255}; // Light Blue
    float angle = 0.0f;
    bool isRunning = true;
    SDL_Event event;

    // --- Core Interactive Loop ---
    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) isRunning = false;
        }
        Render();
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

void Render(width, height, frameBuffer, projectedVertices, ){
        // Clear Framebuffer to Black each frame
    std::fill(frameBuffer.begin(), frameBuffer.end(), 0xFF000000);
    angle += 0.01f; // Increment animation rotation angle

    // Setup temporary screen projection space
    std::vector<float2> projectedVertices(cubeVertices.size());

    // 3. Process and project every vertex dynamically
    for (size_t i = 0; i < cubeVertices.size(); ++i) {
        float3 v = cubeVertices[i];

        // Rotate around Y-axis
        float x1 = v.x * std::cos(angle) - v.z * std::sin(angle);
        float z1 = v.x * std::sin(angle) + v.z * std::cos(angle);
        
        // Rotate around X-axis
        float y2 = v.y * std::cos(angle * 0.5f) - z1 * std::sin(angle * 0.5f);
        float z2 = v.y * std::sin(angle * 0.5f) + z1 * std::cos(angle * 0.5f);

        // Translate the box back away from the camera lens (Z offset)
        float finalZ = z2 - 2.5f; 

        // Perspective Projection Equation
        float fov = 60.0f * M_PI / 180.0f;
        float aspect = static_cast<float>(WIDTH) / HEIGHT;
        float x_proj = x1 / (-finalZ * std::tan(fov / 2.0f));
        float y_proj = y2 / (-finalZ * std::tan(fov / 2.0f) * aspect);

        // Map Viewspace to Pixel Coordinates
        projectedVertices[i].x = (x_proj + 1.0f) * 0.5f * WIDTH;
        projectedVertices[i].y = (1.0f - y_proj) * 0.5f * HEIGHT;
    }

    // 4. Rasterize Edges onto our memory buffer
    for (const auto& edge : cubeEdges) {
        float2 p0 = projectedVertices[edge.first];
        float2 p1 = projectedVertices[edge.second];
        drawLine(static_cast<int>(p0.x), static_cast<int>(p0.y), 
                    static_cast<int>(p1.x), static_cast<int>(p1.y), boxColor, frameBuffer);
    }

    // Display updated graphics memory onto standard SDL texture
    SDL_UpdateTexture(texture, nullptr, frameBuffer.data(), WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}
*/

// testing
void CreateTestImage(){
    const int width = 64;
    const int height = 64;
    vector<vector<float3>> image(width, vector<float3>(height));

    float2 a = float2(0.2f * width, 0.2f * height);
    float2 b = float2(0.7f * width, 0.4f * height);
    float2 c = float2(0.4f * width, 0.8f * height);

    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            float2 p = float2(x, y);
            bool inside = PointInTriangle(a,b,c,p);
            if(inside) image[x][y] = float3(0,0,1);
        }
    }
    
    WriteImageToFile(image, "art.bmp");
}

// RENDER
struct Framebuffer
{
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

void Render(SDL_Renderer* renderer, Framebuffer framebuffer, SDL_Texture* texture){
    // Clear framebuffer
    framebuffer.clear(
        0xFF202020 //opaque very dark gray default color (AARRGGBB)
    );

    // Your software rasterizer goes here.
    //
    // rasterizeTriangle(...);
    //stuff to render e.g:
    
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

void Run(){
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
        cout << "Frame " << frameCount << " render time: " << GetFrameTimeMs() << "ms?" << '\n';
        frameCount++;

        Render(renderer, framebuffer, texture);
        handleEvents();

        if(false){
            doneCounter++;
            if(doneCounter>10000) isDone = false; 
        }
    };

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    /*SDL_DestroyTexture(tex);*/
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    cout << "Starting code!" << endl;
    Run();
    return 0;
}
