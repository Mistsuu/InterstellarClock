#pragma once

#include <d2d1.h>
#include <wincodec.h>
#include <dwrite.h>

#pragma comment (lib, "d2d1")
#pragma comment (lib, "Dwrite")

using namespace std;

/*
		Get the default graphics
		resources of d2d1 lib
*/

template <class T>
void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}


HRESULT InitializeGraphicsResources(
	ID2D1Factory**          ppFactory,
	ID2D1HwndRenderTarget** ppRenderTarget,
	IWICImagingFactory**    ppIWICFactory,
	IDWriteFactory**        ppWriteFactory,
	HWND                    hwnd,
	LONG&                   width,
	LONG&					height
)
{
	/*
		Create factory
	*/
	HRESULT hr = S_OK;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &(*ppFactory));

	if (SUCCEEDED(hr)) 
	{
		// Get size of the hwnd
		RECT hwnd_rc;
		GetClientRect(hwnd, &hwnd_rc);
		width  = hwnd_rc.right;
		height = hwnd_rc.bottom;
		D2D1_SIZE_U size = D2D1::SizeU(width, height);

		// Get the render target of the specified handle (hwnd), with size
		hr = (*ppFactory)->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hwnd, size),
			&(*ppRenderTarget)
		);
	}

	if (SUCCEEDED(hr)) 
	{
		// Initialise COM library
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

		// Get this to get the functions of encoding, decoding stuff
		if (*ppIWICFactory == NULL) 
			hr = CoCreateInstance(
				CLSID_WICImagingFactory,
				NULL,
				CLSCTX_ALL,
				__uuidof(IWICImagingFactory),
				(void**)(&(*ppIWICFactory))
			);	
	}

	if (SUCCEEDED(hr))
	{
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&(*ppWriteFactory))
		);
	}

	return hr;
}

/*
		This function loads bitmap from file

*/

HRESULT LoadBitmapFromFile(
	ID2D1RenderTarget*  pRenderTarget,
	IWICImagingFactory* pIWICFactory,
	PCWSTR              uri,
	ID2D1Bitmap**       ppBitmap
)
{
	// Create an IWICBitmapDecoder through some shit method
	IWICBitmapDecoder*     pDecoder   = NULL;
	IWICBitmapFrameDecode* pSource    = NULL;
	IWICStream*            pStream    = NULL;
	IWICFormatConverter*   pConverter = NULL;

	HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
		uri,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);

	// Retrieve a frame from the image
	if (SUCCEEDED(hr))
	{
		// Create the initial frame.
		hr = pDecoder->GetFrame(0, &pSource);
	}

	if (SUCCEEDED(hr))
	{

		// Convert the image format to 32bppPBGRA
		// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
		hr = pIWICFactory->CreateFormatConverter(&pConverter);

	}

	if (SUCCEEDED(hr))
	{
		hr = pConverter->Initialize(
			pSource,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			NULL,
			0.f,
			WICBitmapPaletteTypeMedianCut
		);
	}

	if (SUCCEEDED(hr))
	{

		// Create a Direct2D bitmap from the WIC bitmap.
		hr = pRenderTarget->CreateBitmapFromWicBitmap(
			pConverter,
			NULL,
			ppBitmap
		);
	}

	SafeRelease(&pDecoder);
	SafeRelease(&pSource);
	SafeRelease(&pStream);
	SafeRelease(&pConverter);

	return hr;
}
