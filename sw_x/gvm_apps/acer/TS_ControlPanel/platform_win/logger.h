//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#pragma once



using namespace System;
using namespace System::IO;
using namespace System::Runtime::InteropServices;


public ref class Win32
{
public:
    [DllImport("kernel32.dll")]
    static bool AllocConsole();

    [DllImport("kernel32.dll")]
    static bool FreeConsole();
};

public ref class Logger
{
public:
    static property Logger^ Instance
    {
        Logger^ get()
        {
            return mInstance;
        }
    }

    void Init();
    void Quit();
    void Write(String^ text);
    void WriteLine(String^ text);

private:
    static Logger^ mInstance = gcnew Logger();

    bool        mUseConsole;
    TextWriter^ mWriter;

    Logger();
    ~Logger();
};