#include "Frame.h"

Frame::Frame() : wxFrame(nullptr, wxID_ANY, "Hello World")
{
    CreateStatusBar();
    SetStatusText("Welcome to wxWidgets!");
}