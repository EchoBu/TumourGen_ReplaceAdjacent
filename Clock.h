#pragma once

#include <windows.h>
#include <time.h>
#include <mmsystem.h>
#pragma comment(lib,"Winmm.lib")

namespace iv
{
	class Clock
	{
	public:
		static double GetCurrentTimeUS(void)
		{
			LARGE_INTEGER large_integer;
			QueryPerformanceFrequency(&large_integer);
			
			double fre = (double)large_integer.QuadPart;

			QueryPerformanceCounter(&large_integer);

			double ct = large_integer.QuadPart * 1000 / fre;
			return ct;
		}

		static double GetCurrentTimeMS(void)
		{
			return clock();
		}
	};

	class Timer
	{
	public:
		Timer(): mFrequency(0.0),mStartCnt(0.0)
		{
			LARGE_INTEGER li;
			QueryPerformanceFrequency(&li);
			mFrequency = li.QuadPart/1000.0;
		}

		void Start()
		{
			LARGE_INTEGER li;
			QueryPerformanceCounter(&li);
			mStartCnt = li.QuadPart;
		}

		double GetElapse(void)
		{
			LARGE_INTEGER li;
			QueryPerformanceCounter(&li);
			return ( li.QuadPart - mStartCnt ) / mFrequency;
		}

	private:
		double mFrequency;
		double mStartCnt;
	};
}