#pragma once

class ImageProcessor
{
public:
    static concurrency::task<void> RunYuvAsync();
    static concurrency::task<void> RunRgbAsync();
};
