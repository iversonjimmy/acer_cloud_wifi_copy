//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "stdafx.h"
#include "logger.h"

using namespace System;
using namespace System::Collections;
using namespace System::IO;
using namespace System::Runtime::InteropServices;
using namespace System::Text;

Logger::Logger()
{
    mWriter = nullptr;

#ifdef _DEBUG
    mUseConsole = true;
#else
    mUseConsole = false;
#endif
}

Logger::~Logger()
{
    // do nothing, memory managed
}

void Logger::Init()
{

	if(mWriter != nullptr)
		return;

    String^ outFile = Environment::GetFolderPath(Environment::SpecialFolder::LocalApplicationData);
    outFile += L"\\iGware\\SyncAgent\\logs\\app";
    Directory::CreateDirectory(outFile);
    outFile += L"\\app.log." + Convert::ToString(DateTime::Today.Year);
    outFile += Convert::ToString(DateTime::Today.Month);
    outFile += Convert::ToString(DateTime::Today.Day);

    mWriter = gcnew StreamWriter(outFile, true, Encoding::UTF8);

    if (mUseConsole == true) {
        Win32::AllocConsole();
    }

    this->WriteLine(L"-------------------------------------");
    this->WriteLine(L"APP START");
    this->WriteLine(L"-------------------------------------");
}

void Logger::Quit()
{
    mWriter->Flush();
    mWriter->Close();

    if (mUseConsole == true) {
        Win32::FreeConsole();
    }
}

void Logger::Write(String^ text)
{
    if (mUseConsole == true) {
        Console::Write(text);
    }
    mWriter->Write(text);
    mWriter->Flush();
}

void Logger::WriteLine(String^ text)
{
    if (mUseConsole == true) {
        Console::WriteLine(text);
    }
    mWriter->WriteLine(text);
    mWriter->Flush();
}
