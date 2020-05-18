/*
	Brute Force Processing a Mandelbrot Renderer
	"Dammit Moros & Saladin, you guys keep making tools, I'll have nothing left to video..." - javidx9
	License (OLC-3)
	~~~~~~~~~~~~~~~
	Copyright 2018-2020 OneLoneCoder.com
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:
	1. Redistributions or derivations of source code must retain the above
	copyright notice, this list of conditions and the following disclaimer.
	2. Redistributions or derivative works in binary form must reproduce
	the above copyright notice. This list of conditions and the following
	disclaimer must be reproduced in the documentation and/or other
	materials provided with the distribution.
	3. Neither the name of the copyright holder nor the names of its
	contributors may be used to endorse or promote products derived
	from this software without specific prior written permission.
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
	Relevant Video: https://youtu.be/PBvLs88hvJ8
	Links
	~~~~~
	YouTube:	https://www.youtube.com/javidx9
				https://www.youtube.com/javidx9extra
	Discord:	https://discord.gg/WhwHUMV
	Twitter:	https://www.twitter.com/javidx9
	Twitch:		https://www.twitch.tv/javidx9
	GitHub:		https://www.github.com/onelonecoder
	Patreon:	https://www.patreon.com/javidx9
	Homepage:	https://www.onelonecoder.com
	Community Blog: https://community.onelonecoder.com
	Author
	~~~~~~
	David Barr, aka javidx9, ©OneLoneCoder 2018, 2019, 2020
*/

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#include <condition_variable>
#include <atomic>
#include <complex>
#include <cstdlib>
#include <immintrin.h>

class olcFractalExplorer : public olc::PixelGameEngine
{
public:
	olcFractalExplorer()
	{
		sAppName = "Brute force processing";
	}

	int* pFractal = nullptr;
	int nMode = 0;
	int nIterations = 64;

public:
	bool OnUserCreate() override
	{
		pFractal = new int[ScreenWidth() * ScreenHeight()]{ 0 };
		return true;
	}

	// Method 1) - Super simple, no effort at optimising
	void CreateFractalBasic(const olc::vi2d& pix_tl, const olc::vi2d& pix_br, const olc::vd2d& frac_tl, const olc::vd2d& frac_br, const int iterations)
	{
		double x_scale = (frac_br.x - frac_tl.x) / (double(pix_br.x) - double(pix_tl.x));
		double y_scale = (frac_br.y - frac_tl.y) / (double(pix_br.y) - double(pix_tl.y));

		for (int y = pix_tl.y; y < pix_br.y; y++)
		{
			for (int x = pix_tl.x; x < pix_br.x; x++)
			{
				std::complex<double> c(x * x_scale + frac_tl.x, y * y_scale + frac_tl.y);
				std::complex<double> z(0, 0);

				int n = 0;
				while (abs(z) < 2.0 && n < iterations)
				{
					z = (z * z) + c;
					n++;
				}

				pFractal[y * ScreenWidth() + x] = n;
			}
		}
	}

	// Method 2) - Attempt to pre-calculate as much as possible, and reduce
	// repeated multiplications
	void CreateFractalPreCalculate(const olc::vi2d& pix_tl, const olc::vi2d& pix_br, const olc::vd2d& frac_tl, const olc::vd2d& frac_br, const int iterations)
	{
		double x_scale = (frac_br.x - frac_tl.x) / (double(pix_br.x) - double(pix_tl.x));
		double y_scale = (frac_br.y - frac_tl.y) / (double(pix_br.y) - double(pix_tl.y));

		double x_pos = frac_tl.x;
		double y_pos = frac_tl.y;

		int y_offset = 0;
		int row_size = pix_br.x - pix_tl.x;

		int x, y, n;
		std::complex<double> c, z;

		for (y = pix_tl.y; y < pix_br.y; y++)
		{
			x_pos = frac_tl.x;
			for (x = pix_tl.x; x < pix_br.x; x++)
			{
				c = { x_pos, y_pos };
				z = { 0,0 };

				n = 0;
				while (abs(z) < 2.0 && n < iterations)
				{
					z = (z * z) + c;
					n++;
				}

				pFractal[y_offset + x] = n;
				x_pos += x_scale;
			}

			y_pos += y_scale;
			y_offset += row_size;
		}
	}


	bool OnUserUpdate(float fElapsedTime) override
	{
		/*

		Inital code olc Framework

		// called once per frame
		for (int x = 0; x < ScreenWidth(); x++)
			for (int y = 0; y < ScreenHeight(); y++)
				Draw(x, y, olc::Pixel(rand() % 255, rand() % 255, rand() % 255));
		return true;
		*/

		// Get mouse location this frame
		olc::vd2d vMouse = { (double)GetMouseX(), (double)GetMouseY() };

		// Handle Pan & Zoom
		if (GetMouse(2).bPressed)
		{
			vStartPan = vMouse;
		}

		if (GetMouse(2).bHeld)
		{
			vOffset -= (vMouse - vStartPan) / vScale;
			vStartPan = vMouse;
		}

		olc::vd2d vMouseBeforeZoom;
		ScreenToWorld(vMouse, vMouseBeforeZoom);

		if (GetKey(olc::Key::Q).bHeld || GetMouseWheel() > 0) vScale *= 1.1;
		if (GetKey(olc::Key::A).bHeld || GetMouseWheel() < 0) vScale *= 0.9;

		olc::vd2d vMouseAfterZoom;
		ScreenToWorld(vMouse, vMouseAfterZoom);
		vOffset += (vMouseBeforeZoom - vMouseAfterZoom);

		olc::vi2d pix_tl = { 0,0 };
		olc::vi2d pix_br = { ScreenWidth(), ScreenHeight() };
		olc::vd2d frac_tl = { -2.0, -1.0 };
		olc::vd2d frac_br = { 1.0, 1.0 };

		ScreenToWorld(pix_tl, frac_tl);
		ScreenToWorld(pix_br, frac_br);

		// Handle User Input
		if (GetKey(olc::K1).bPressed) nMode = 0;
		if (GetKey(olc::K2).bPressed) nMode = 1;
		if (GetKey(olc::K3).bPressed) nMode = 2;
		if (GetKey(olc::K4).bPressed) nMode = 3;
		if (GetKey(olc::K5).bPressed) nMode = 4;
		if (GetKey(olc::K6).bPressed) nMode = 5;
		if (GetKey(olc::UP).bPressed) nIterations += 64;
		if (GetKey(olc::DOWN).bPressed) nIterations -= 64;
		if (nIterations < 64) nIterations = 64;


		// START TIMING
		auto tp1 = std::chrono::high_resolution_clock::now();

		// Do the computation
		switch (nMode)
		{
		case 0: CreateFractalBasic(pix_tl, pix_br, frac_tl, frac_br, nIterations); break;
		case 1: CreateFractalPreCalculate(pix_tl, pix_br, frac_tl, frac_br, nIterations); break;
		// case 2: CreateFractalNoComplex(pix_tl, pix_br, frac_tl, frac_br, nIterations); break;
		// case 3: CreateFractalIntrinsics(pix_tl, pix_br, frac_tl, frac_br, nIterations); break;
		// case 4: CreateFractalThreads(pix_tl, pix_br, frac_tl, frac_br, nIterations); break;
		// case 5: CreateFractalThreadPool(pix_tl, pix_br, frac_tl, frac_br, nIterations); break;
		}

		// STOP TIMING
		auto tp2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsedTime = tp2 - tp1;


		// Render result to screen
		for (int y = 0; y < ScreenHeight(); y++)
		{
			for (int x = 0; x < ScreenWidth(); x++)
			{
				int i = pFractal[y * ScreenWidth() + x];
				float n = (float)i;
				float a = 0.1f;
				// Thank you @Eriksonn - Wonderful Magic Fractal Oddball Man
				Draw(x, y, olc::PixelF(0.5f * sin(a * n) + 0.5f, 0.5f * sin(a * n + 2.094f) + 0.5f, 0.5f * sin(a * n + 4.188f) + 0.5f));
			}
		}

		// Render UI
		switch (nMode)
		{
		case 0: DrawString(0, 0, "1) Naive Method", olc::WHITE, 3); break;
		case 1: DrawString(0, 0, "2) Precalculate Method", olc::WHITE, 3); break;
		// case 2: DrawString(0, 0, "3) Hand-code Maths Method", olc::WHITE, 3); break;
		// case 3: DrawString(0, 0, "4) Vector Extensions (AVX2) Method", olc::WHITE, 3); break;
		// case 4: DrawString(0, 0, "5) Threads Method", olc::WHITE, 3); break;
		// case 5: DrawString(0, 0, "6) ThreadPool Method", olc::WHITE, 3); break;
		}

		DrawString(0, 30, "Time Taken: " + std::to_string(elapsedTime.count()) + "s", olc::WHITE, 3);
		DrawString(0, 60, "Iterations: " + std::to_string(nIterations), olc::WHITE, 3);
		return !(GetKey(olc::Key::ESCAPE).bPressed);
	}
	
	// Pan & Zoom variables
	olc::vd2d vOffset = { 0.0, 0.0 };
	olc::vd2d vStartPan = { 0.0, 0.0 };
	olc::vd2d vScale = { 1280.0 / 2.0, 720.0 };


	// Convert coordinates from World Space --> Screen Space
	void WorldToScreen(const olc::vd2d& v, olc::vi2d& n)
	{
		n.x = (int)((v.x - vOffset.x) * vScale.x);
		n.y = (int)((v.y - vOffset.y) * vScale.y);
	}

	// Convert coordinates from Screen Space --> World Space
	void ScreenToWorld(const olc::vi2d& n, olc::vd2d& v)
	{
		v.x = (double)(n.x) / vScale.x + vOffset.x;
		v.y = (double)(n.y) / vScale.y + vOffset.y;
	}
};

// std::atomic<int> olcFractalExplorer::nWorkerComplete = 0;

int main()
{
	olcFractalExplorer demo;
	if (demo.Construct(1280, 720, 1, 1, false, false))
		demo.Start();
	return 0;

	return 0;
}