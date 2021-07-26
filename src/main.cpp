#include "GraphicsResources.h"
#include "WindowsStuffs.h"
#include "Calc.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <chrono>
#include <ctime>
using namespace std;

/*
	Some global variables
*/

// Number of '4d' rays shooting out of second hand
const int NMAXRAYS = 25;

// Current time variable
long long millisec_since_epoch;
double    hour;
double    minute;
double    second;

// All necessary stuff to get drawing
ID2D1Factory*          pFac = NULL;
ID2D1HwndRenderTarget* pRen = NULL;
IWICImagingFactory*    pImg = NULL;
IDWriteFactory*        pWrt = NULL;
LONG                   width, height;

// THE TEXT
IDWriteTextFormat*    pGlobalTxtFormat = NULL;
ID2D1SolidColorBrush* pTextBrush = NULL;

// THE CLOCK
D2D1_POINT_2F         hourHand[2];
D2D1_POINT_2F         minuteHand[2];
D2D1_POINT_2F         secondHand[2];
D2D1_POINT_2F         hourMark[12][2];
D2D1_POINT_2F         quarterMark[48][2];
D2D1_POINT_2F         center;
D2D1_ELLIPSE          dot;
D2D1_POINT_2F         fourDimensionalRays[NMAXRAYS][2];
ID2D1SolidColorBrush* pClockBrush = NULL;
static double         phase = 0;


// THE BACKGROUND
D2D1_COLOR_F          backgroundColor;

/*
	Some temp variables
*/
MSG           msg = {};
D2D1_COLOR_F  color;
D2D1_POINT_2F point;
int           ct = 10001;

/*
	Configurations -- Yeah I know some of the config looks fucking weird...
*/
static double clockX = 960, clockY = 700;
static double dotOffX = 5, dotOffY = 20;
static double tiltFactor           = 2;
static double dotSize              = 6;
static double secondHandLength     = 204;
static double minuteHandLength     = 128;
static double hourHandLength       = 80;
static double secondTailLength     = 20;
static double minuteTailLength     = 25;
static double hourTailLength       = 20;
static double hourHandThickness    = 4.0;
static double minuteHandThickness  = 6.0;
static double secondHandThickness  = 1.6;
static double clockSize            = 230;
static double hourMarkLength       = 20;
static double hourMarkThickness    = 3.0;
static double quarterMarkThickness = 1.0;
static double hourMarkOpacity      = 1.0;
static double quarterMarkOpacity   = 0.5;
static double quarterMarkLength    = 10;
static int    fps                  = 30;
static double timeZone             = 7;

inline void getCurrentTime()
{
	millisec_since_epoch = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
	hour                 = fmod((millisec_since_epoch / 3600000.0) + timeZone, 12.0);
	minute               = fmod((millisec_since_epoch / 60000.0),              60.0);
	second               = fmod((millisec_since_epoch / 1000.0),               60.0);
}

/*
	This one is used to load configs
*/
const string configFilename = "config.txt";
bool LoadConfigs()
{
	fstream configFile;
	configFile.open(configFilename, fstream::in);
	if (!configFile.is_open())
		return TRUE;

	string line;
	int    commentPos;
	int    spacePos;
	string attribute;
	double value;
	while (getline(configFile, line)) {
		if ((commentPos = line.find('#')) != -1) {
			line = line.substr(0, commentPos);
		}

		if ((spacePos = line.find(' ')) != -1) {
			attribute = line.substr(0, spacePos);
			istringstream iss(line.substr(spacePos + 1));
			iss >> value;

			if (attribute == "clockX")               clockX = value;
			if (attribute == "clockY")               clockY = value;
			if (attribute == "dotOffX")		         dotOffX = value;
			if (attribute == "dotOffY")		         dotOffY = value;
			if (attribute == "tiltFactor")           tiltFactor = value;
			if (attribute == "dotSize")              dotSize = value;
			if (attribute == "secondHandLength")     secondHandLength = value;
			if (attribute == "minuteHandLength")     minuteHandLength = value;
			if (attribute == "hourHandLength")       hourHandLength = value;
			if (attribute == "secondTailLength")     secondTailLength = value;
			if (attribute == "minuteTailLength")     minuteTailLength = value;
			if (attribute == "hourTailLength")       hourTailLength = value;
			if (attribute == "hourHandThickness")    hourHandThickness = value;
			if (attribute == "minuteHandThickness")  minuteHandThickness = value;
			if (attribute == "secondHandThickness")  secondHandThickness = value;
			if (attribute == "clockSize")            clockSize = value;
			if (attribute == "hourMarkLength")       hourMarkLength = value;
			if (attribute == "hourMarkThickness")    hourMarkThickness = value;
			if (attribute == "quarterMarkThickness") quarterMarkThickness = value;
			if (attribute == "hourMarkOpacity")      hourMarkOpacity = value;
			if (attribute == "quarterMarkOpacity")   quarterMarkOpacity = value;
			if (attribute == "quarterMarkLength")    quarterMarkLength = value;
			if (attribute == "fps")                  fps = (int)value;
			if (attribute == "timeZone")             timeZone = value;
		}

	}

	return TRUE;
}

/*
	This one creates drawing stuff,
	like ellipses, brushes (for colors),
	rectangles, bitmaps...
*/

void CreateDrawingStuff(ID2D1HwndRenderTarget* pRen, IDWriteFactory* pWrt)
{
	/*
		THE BACKGROUND
	*/
	backgroundColor = D2D1::ColorF(D2D1::ColorF::Black);

	/*
		THE CLOCK
	*/
	center.x    = clockX;
	center.y    = clockY;

	dot.point.x = center.x - dotOffX;
	dot.point.y = center.y - dotOffY;
	dot.radiusX = dotSize;
	dot.radiusY = dotSize / tiltFactor;

	phase = get12HourMarkOnClock(
		dot.point, center, 
		clockSize, clockSize / tiltFactor
	);

	for (int hr = 0; hr < 12; ++hr) {
		hourMark[hr][0].x = center.x + clockSize              * cos(PI / 6 * hr + phase);
		hourMark[hr][0].y = center.y + clockSize / tiltFactor * sin(PI / 6 * hr + phase);

		hourMark[hr][1].x = (hourMark[hr][0].x - dot.point.x) * hourMarkLength / clockSize + hourMark[hr][0].x;
		hourMark[hr][1].y = (hourMark[hr][0].y - dot.point.y) * hourMarkLength / clockSize + hourMark[hr][0].y;
		
		for (int quar = 0; quar <= 3; ++quar) {
			quarterMark[hr * 4 + quar][0].x = center.x + clockSize              * cos(PI / 6 * ((quar + 1) * 0.2 + hr * 1.0) + phase);
			quarterMark[hr * 4 + quar][0].y = center.y + clockSize / tiltFactor * sin(PI / 6 * ((quar + 1) * 0.2 + hr * 1.0) + phase);

			quarterMark[hr * 4 + quar][1].x = (quarterMark[hr * 4 + quar][0].x - dot.point.x) * quarterMarkLength / clockSize + quarterMark[hr * 4 + quar][0].x;
			quarterMark[hr * 4 + quar][1].y = (quarterMark[hr * 4 + quar][0].y - dot.point.y) * quarterMarkLength / clockSize + quarterMark[hr * 4 + quar][0].y;
		}
	}

	pRen->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::BlanchedAlmond), &pClockBrush);

	/*
		THE TEXT - maybe later I want to add text...
	*/
	HRESULT hr = pWrt->CreateTextFormat(
		L"Amatic SC",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		25,
		L"", //locale
		&pGlobalTxtFormat
	);

	if (SUCCEEDED(hr)) {
		pGlobalTxtFormat->SetTextAlignment     (DWRITE_TEXT_ALIGNMENT_CENTER);
		pGlobalTxtFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}
	pRen->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pTextBrush);
}

/*
	This one handles loop's
	logic
*/
void HandleLoopLogic()
{
	getCurrentTime();

	/*
	*	Configure the clock hands
	*/ 

	// Configure hour hand
	point.x         = center.x + clockSize              * cos(PI / 6 * hour + phase);
	point.y         = center.y + clockSize / tiltFactor * sin(PI / 6 * hour + phase);
	hourHand[1].x   = (point.x - dot.point.x) * hourHandLength / clockSize + dot.point.x;
	hourHand[1].y   = (point.y - dot.point.y) * hourHandLength / clockSize + dot.point.y;
	hourHand[0].x   = (dot.point.x - point.x) * hourTailLength / clockSize + dot.point.x;
	hourHand[0].y   = (dot.point.y - point.y) * hourTailLength / clockSize + dot.point.y;

	// Configure minute hand
	point.x         = center.x + clockSize              * cos(PI / 30 * minute + phase);
	point.y         = center.y + clockSize / tiltFactor * sin(PI / 30 * minute + phase);
	minuteHand[1].x = (point.x - dot.point.x) * minuteHandLength / clockSize + dot.point.x;
	minuteHand[1].y = (point.y - dot.point.y) * minuteHandLength / clockSize + dot.point.y;
	minuteHand[0].x = (dot.point.x - point.x) * minuteTailLength / clockSize + dot.point.x;
	minuteHand[0].y = (dot.point.y - point.y) * minuteTailLength / clockSize + dot.point.y;

	// Configure second hand
	point.x         = center.x + clockSize              * cos(PI / 30 * second + phase);
	point.y         = center.y + clockSize / tiltFactor * sin(PI / 30 * second + phase);
	secondHand[1].x = (point.x - dot.point.x) * secondHandLength / clockSize + dot.point.x;
	secondHand[1].y = (point.y - dot.point.y) * secondHandLength / clockSize + dot.point.y;
	secondHand[0].x = (dot.point.x - point.x) * secondTailLength / clockSize + dot.point.x;
	secondHand[0].y = (dot.point.y - point.y) * secondTailLength / clockSize + dot.point.y;

	/* 
		Configure random '4d' rays shooting out of the clock hands
	*/
	for (int i = 0; i < NMAXRAYS; ++i) {
		float ratioBetweenSecondEndWithDot = rand() * 1.0 / RAND_MAX;

		fourDimensionalRays[i][0].x = (secondHand[1].x - dot.point.x) * ratioBetweenSecondEndWithDot + dot.point.x;
		fourDimensionalRays[i][0].y = (secondHand[1].y - dot.point.y) * ratioBetweenSecondEndWithDot + dot.point.y;

		fourDimensionalRays[i][1].x = fourDimensionalRays[i][0].x;
		fourDimensionalRays[i][1].y = 0;
	}
}

/*
	This function use for drawing stuff
*/

void DrawFunc(ID2D1HwndRenderTarget* pRen)
{
	pRen->BeginDraw();

	// <--- Drawing code starts here --->
	pRen->Clear(backgroundColor);

	pClockBrush->SetOpacity(1.0f);
	pRen->FillEllipse(dot, pClockBrush);
	pRen->DrawLine(hourHand[0],   hourHand[1],   pClockBrush, hourHandThickness);
	pRen->DrawLine(minuteHand[0], minuteHand[1], pClockBrush, minuteHandThickness);
	pRen->DrawLine(secondHand[0], secondHand[1], pClockBrush, secondHandThickness);

	pClockBrush->SetOpacity(hourMarkOpacity);
	for (int hr = 0; hr < 12; ++hr) {
		pRen->DrawLine(hourMark[hr][0], hourMark[hr][1], pClockBrush, hourMarkThickness);
	}

	pClockBrush->SetOpacity(quarterMarkOpacity);
	for (int quar = 0; quar < 48; ++quar) {
		pRen->DrawLine(quarterMark[quar][0], quarterMark[quar][1], pClockBrush, quarterMarkThickness);
	}

	for (int i = 0; i < NMAXRAYS; ++i) {
		pClockBrush->SetOpacity(rand() * 1.0 / RAND_MAX);
		pRen->DrawLine(fourDimensionalRays[i][0], fourDimensionalRays[i][1], pClockBrush, rand() * 0.8 / RAND_MAX);
	}

	// <--- End drawing, only drawing in this region --> 
	// <-- Kidding, you could break the rule -->
	// <-- But you know, for nothing -->
	pRen->EndDraw();
}

//int main()
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	srand(time(0));

	if (LoadConfigs() == FALSE) {
		MessageBox(
			NULL,
			(LPCWSTR)L"Error in loading configs!",
			(LPCWSTR)L"Error",
			MB_OK
		);
		return -4;
	}

	HWND hDesktop;
	if (GetBackgroundHandle(hDesktop) == FALSE) {
		MessageBox(
			NULL,
			(LPCWSTR)L"Error in finding your damn desktop!",
			(LPCWSTR)L"Error",
			MB_OK
		);
		return -2;
	}

	HRESULT hr = InitializeGraphicsResources(&pFac, &pRen, &pImg, &pWrt, hDesktop, width, height);
	if (FAILED(hr)) {
		MessageBox(
			NULL,
			(LPCWSTR)L"Cannot initialize graphics resources!",
			(LPCWSTR)L"Error",
			MB_OK
		);
		return -1;
	}

	CreateDrawingStuff(pRen, pWrt);

	while (true) {
		// To toggle callback function
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		HandleLoopLogic();
		DrawFunc(pRen);

		Sleep(1000 / fps);
	}

Cleanup:
	SafeRelease(&pFac);
	SafeRelease(&pRen);
	SafeRelease(&pImg);

	return 0;
}
