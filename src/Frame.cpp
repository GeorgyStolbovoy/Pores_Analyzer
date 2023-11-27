#include "Frame.h"
#include <wx/filedlg.h>
#include <boost/gil/io/read_image.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/tiff.hpp>
#include <boost/gil/extension/io/bmp.hpp>
#include <boost/json.hpp>
#include <wx/msgdlg.h>

wxWindowID Frame::soa_id = wxWindow::NewControlId(), Frame::sock_id = wxWindow::NewControlId();

wxBEGIN_EVENT_TABLE(Frame, wxWindow)
    EVT_MENU(wxID_OPEN, Frame::Load_Image)
	EVT_MENU(soa_id, Frame::OnSoa)
#ifdef VIEWER
	EVT_SOCKET(sock_id, Frame::OnSocket)
#endif
    EVT_MENU(wxID_EXIT, Frame::Exit)
wxEND_EVENT_TABLE()

void Frame::Load_Image(wxCommandEvent& event)
{
    wxFileDialog dlg{this, "Выберите изображение", wxEmptyString, wxEmptyString, "Images (png, jpg, tiff)|*.png;*.jpg;*.jpeg;*.tiff", wxFD_OPEN|wxFD_FILE_MUST_EXIST};
    if (dlg.ShowModal() == wxID_OK)
    {
        if (auto&& path = dlg.GetPath(); path.EndsWith(".png"))
            gil::read_and_convert_image(path.c_str().AsChar(), ImageWindow::image, gil::png_tag());
        else if (path.EndsWith(".jpg") || path.EndsWith(".jpeg"))
            gil::read_and_convert_image(path.c_str().AsChar(), ImageWindow::image, gil::jpeg_tag());
        else if (path.EndsWith(".tiff"))
            gil::read_and_convert_image(path.c_str().AsChar(), ImageWindow::image, gil::tiff_tag());
        else if (path.EndsWith(".bmp"))
            gil::read_and_convert_image(path.c_str().AsChar(), ImageWindow::image, gil::bmp_tag());
        m_image->OnSizeLinear();
#ifndef VIEWER
        m_curves->path_histogram = std::nullopt;
#endif
        Refresh();
    }
}

#ifdef VIEWER
	void Frame::OnSocket(wxSocketEvent& event)
	{
        namespace json = boost::json;
        
        char buf[64*1024];
		sock->Read(buf, 64*1024);
		json::array arr = json::parse(std::string_view{buf, std::strchr(buf, ']') + 1}).as_array();
		for (uint16_t i = 0; i < 256; ++i)
            m_image->histogram[i] = arr[i].as_int64();
		m_image->OnSizeLinear();
	    m_image->Refresh();
	    m_image->Update();
	}
#endif

void Frame::OnSoa(wxCommandEvent& event)
{
	if (event.IsChecked())
	{
        wxIPV4address addr;
        addr.LocalHost();
#ifdef VIEWER
		addr.Service(16800);
#endif
		sock = new wxDatagramSocket(addr);
#ifdef VIEWER
        sock->SetEventHandler(*this, sock_id);
	    sock->SetNotify(wxSOCKET_INPUT_FLAG);
	    sock->Notify(true);
#endif
    }
    else
    {
#ifdef VIEWER
        for (uint16_t i = 0; i < 256; ++i)
            m_image->histogram[i] = i;
        m_image->OnSizeLinear();
	    m_image->Refresh();
	    m_image->Update();
#endif
	    SoaCleanup();
    }
}

#ifndef VIEWER
void Frame::RefreshImage(ImageWindow::Histogram_t&& hist)
{
    namespace json = boost::json;

    if (m_menuItem4->IsChecked())
    {
        json::array arr;
        for (uint8_t i: hist)
            arr.push_back(i);
        std::string jn = json::serialize(arr);
        wxIPV4address addr;
        addr.LocalHost();
		addr.Service(16800);
	    sock->SendTo(addr, jn.c_str(), jn.size());
    }
    m_image->histogram = std::move(hist);
    m_image->OnSizeLinear();
    m_image->Refresh();
    m_image->Update();
}
#endif

void Frame::SoaCleanup()
{
    sock->Destroy();
}

Frame::~Frame()
{
	if (m_menuItem4->IsChecked())
	    SoaCleanup();
}
