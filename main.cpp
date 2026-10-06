#include <SDL.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <string>

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

// actual
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

int main(){
    CreateTestImage();
    cout << "image made succesffuly" << endl;
    return 0;
}
