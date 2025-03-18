#pragma once
#include "framework.h"

#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <windows.h>

class RCStreamBuffer : public std::streambuf {
private:
    std::streambuf*    originalBuffer;
    HANDLE             consoleHandle;
    std::ostringstream buffer;
    WORD               color;

public:
    RCStreamBuffer(std::ostream& stream, WORD textColor) : 
        originalBuffer(stream.rdbuf()), color(textColor) 
    {
        consoleHandle = GetStdHandle(&stream == &std::cerr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
        stream.rdbuf(this);  // Redirect stream to this buffer
    }

    ~RCStreamBuffer() {
        std::cout.rdbuf(originalBuffer);  // Restore original buffer
    }

protected:
    virtual int overflow(int c) override {
        if (c == EOF) return EOF;

        buffer.put(static_cast<char>(c));

        // If newline detected, process and print text
        if (c == '\n') {
            processBuffer();
        }

        return c;
    }

private:
    void processBuffer() {
        std::string text = buffer.str();
        buffer.str("");  // Clear buffer

        // Change color based on keywords
        if (text.find("[ERROR]") != std::string::npos) {
            SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);
        }
        else if (text.find("[WARN]") != std::string::npos) {
            SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        }
        else {
            SetConsoleTextAttribute(consoleHandle, color);
        }

        // Print the modified text
        DWORD written;
        WriteConsoleA(consoleHandle, text.c_str(), text.size(), &written, nullptr);

        // Reset color
        SetConsoleTextAttribute(consoleHandle, color);
    }
};

class DebugConsole {
public:
	static void init() {
		AllocConsole();

		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout); // Redirect stdout
		freopen_s(&fp, "CONIN$", "r", stdin);  // Redirect stdin
		freopen_s(&fp, "CONERR$", "w", stderr); // Redirect stderr
	}

	static void del() {
		FreeConsole();
	}
};