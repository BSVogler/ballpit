//
//  wxVolumeRenderer.h
//  ballpit
//
//  Created by Benedikt Stefan Vogler on 19.08.20.
//

#ifndef wxVolumeRenderer_h
#define wxVolumeRenderer_h

#include <wx/rawbmp.h>
class wxVolumeRenderer : public wxPanel {
	wxBitmap image;
	
public:
	wxVolumeRenderer(wxFrame* parent, wxBitmapType format);
	
	void paintEvent(wxPaintEvent & evt);
	void paintNow();
	void render(wxDC& dc);

	DECLARE_EVENT_TABLE()
};

#endif /* wxVolumeRenderer_h */
