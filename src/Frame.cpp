#include "Frame.h"
#include <wx/filedlg.h>
#include <atomic>
#include <boost/gil/io/read_image.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/tiff.hpp>
#include <boost/gil/extension/io/bmp.hpp>
#include <boost/interprocess/containers/string.hpp>
//#include <boost/interprocess/offset_ptr.hpp>
#include <boost/json.hpp>
#include <wx/msgdlg.h>

wxWindowID Frame::soa_id = wxWindow::NewControlId();

wxBEGIN_EVENT_TABLE(Frame, wxWindow)
    EVT_MENU(wxID_OPEN, Frame::Load_Image)
	EVT_MENU(soa_id, Frame::OnSoa)
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
	void Frame::SoaCallback()
	{
		wxMessageBox(wxT("Message box text"), wxT("Message box caption"),wxNO_DEFAULT|wxYES_NO|wxCANCEL|wxICON_INFORMATION);	
	}
#endif

void Frame::OnSoa(wxCommandEvent& event)
{
    namespace process = boost::interprocess;

	if (event.IsChecked())
	{
		segment = process::managed_shared_memory{process::open_or_create, shared_memory_name, 65536};
#ifdef VIEWER
        segment.construct<process::offset_ptr<void>>(shared_callback_name)(&Frame::SoaCallback);
#endif
        //boost::interprocess::shared_memory_object::remove(shared_memory_name);
    }
    else
	    SoaCleanup();
}

void Frame::SoaCleanup()
{
    namespace process = boost::interprocess;

#ifndef VIEWER
    if (std::atomic_flag *flag = segment.find<std::atomic_flag>(shared_flag_name).first; flag)
    {
        //if (flag->test_and_set())
		//	flag->wait(true);
	    segment.destroy<
            process::basic_string<
				char,
    			std::char_traits<char>,
    			process::allocator<char, process::managed_shared_memory::segment_manager>
    		>
    	>(shared_string_name);
	    segment.destroy<std::atomic_flag>(shared_flag_name);
    }
	process::shared_memory_object::remove(shared_memory_name);
#else
	segment.destroy<process::offset_ptr<void>>(shared_callback_name);
#endif
}

#ifndef VIEWER
void Frame::RefreshImage(ImageWindow::Histogram_t&& hist)
{
    namespace process = boost::interprocess;
    namespace json = boost::json;
    using allocator_t = process::allocator<char, process::managed_shared_memory::segment_manager>;
    using string_t = process::basic_string<char, std::char_traits<char>, allocator_t>;

    if (m_menuItem4->IsChecked())
    {
        std::atomic_flag *flag = segment.find_or_construct<std::atomic_flag>(shared_flag_name)();
        //flag->wait(true);
        //flag->test_and_set();
        if (process::offset_ptr<void>* callback = segment.find<process::offset_ptr<void>>(shared_callback_name).first; callback)
        {
	        json::array arr;
	        for (uint8_t i: hist)
	            arr.push_back(i);
            std::string jn = json::serialize(arr);
		    if (string_t* str = segment.find<string_t>(shared_string_name).first; str)
	            str->assign(jn.c_str());
	        else
		        segment.construct<string_t>(shared_string_name)(jn.c_str(), allocator_t(segment.get_segment_manager()));
            reinterpret_cast<void(*)()>(callback->get())();
        }
    }
    m_image->histogram = std::move(hist);
    m_image->OnSizeLinear();
    m_image->Refresh();
    m_image->Update();
}
#endif

Frame::~Frame()
{
	if (m_menuItem4->IsChecked())
	    SoaCleanup();
}
