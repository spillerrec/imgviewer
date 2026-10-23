/*
	This file is part of imgviewer.

	imgviewer is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	imgviewer is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with imgviewer.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COLOR_MANAGER_H
#define COLOR_MANAGER_H

#include <lcms2.h>
#include <QString>
#include <vector>

class QScreen;


//TODO: see if we can safely use std::unique_ptr to handle it, or make our own class
class ColorTransform{
	private:
		cmsHTRANSFORM transform{ nullptr };
		
	public:
		ColorTransform() { }
		ColorTransform( cmsHTRANSFORM transform ) : transform(transform) { }
		ColorTransform( const ColorTransform& copy ) = delete;
		ColorTransform( ColorTransform&& other ){
			transform = other.transform;
			other.transform = nullptr;
		}
		~ColorTransform(){ if(transform) cmsDeleteTransform( transform ); }
		operator bool() const{ return transform; }
		
		void execute( const void* input_buffer, void* output_buffer, unsigned size )
			{ cmsDoTransform( transform, input_buffer, output_buffer, size ); }
};

//cICP chunk values (ITU-T H.273 - 8.1 table 2)
enum class ColorPrimaries : uint8_t{
    RESERVED        = 0,
    BT709           = 1,   // Also sRGB / IEC 61966-2-1
    UNSPECIFIED     = 2,
    RESERVED_3      = 3,
    BT470M          = 4,   // US NTSC 1953 (System M)
    BT470BG         = 5,   // System B,G / BT.601-7 625
    BT601_525       = 6,   // BT.601-7 525 / SMPTE ST 170 (same as 7)
    SMPTE240        = 7,   // Same chromaticities as 6
    FILM            = 8,   // Generic film (Wratten filters, Illuminant C)
    BT2020          = 9,   // Also BT.2100
    XYZ             = 10,  // SMPTE ST 428-1 / CIE 1931 XYZ
    SMPTE431        = 11,  // SMPTE RP 431-2 (SMPTE C)
    SMPTE432        = 12,  // SMPTE EG 432-1 (DCI P3)
    // 13–21: Reserved
    PRIMARIES_22    = 22,  // No corresponding industry specification
    // 23–255: Reserved
};

//ITU-T H.273 - 8.2 table 3
enum class TransferFunction : uint8_t{
    RESERVED              = 0,
    BT709                 = 1,   // Also BT.1361 conventional gamut
    UNSPECIFIED           = 2,
    RESERVED_3            = 3,
    GAMMA_2_2             = 4,   // BT.470-6 System M / NTSC 1953
    GAMMA_2_8             = 5,   // BT.470-6 System B,G
    BT601                 = 6,   // BT.601-7 / SMPTE ST 170 (same as 1, 14, 15)
    SMPTE_240             = 7,   // SMPTE ST 240M
    LINEAR                = 8,
    LOG_100               = 9,   // Logarithmic (100:1 range)
    LOG_100_SQRT10        = 10,  // Logarithmic (100*sqrt(10):1 range)
    IEC61966_2_4          = 11,  // IEC 61966-2-4
    BT1361_EG             = 12,  // BT.1361-0 extended colour gamut
    SRGB                  = 13,  // sRGB / sYCC (depends on MatrixCoefficients)
    BT2020_10BIT          = 14,  // Same curve as BT.709/BT.601
    BT2020_12BIT          = 15,  // Same curve as BT.709/BT.601
    SMPTE_ST_2084         = 16,  // PQ / BT.2100 perceptual quantization
    SMPTE_ST_428          = 17,
    HLG                   = 18,  // ARIB STD-B67 / BT.2100 hybrid log-gamma
};
// 19–255: Reserved for future use

//ITU-T H.273 - 8.3 table 4
enum class MatrixCoefficients : uint8_t{
	IDENTITY             = 0,   // Identity (GBR/RGB or XYZ)
	BT709                = 1,   // KR=0.2126; KB=0.0722
	UNSPECIFIED          = 2,
	RESERVED_3           = 3,
	FCC                  = 4,   // KR=0.30; KB=0.11 (US FCC)
	BT601_625            = 5,   // KR=0.299; KB=0.114 (BT.601-7 625 / BT.470-6 B,G)
	BT601_525            = 6,   // KR=0.299; KB=0.114 (BT.601-7 525) — functionally same as 5
	SMPTE240M            = 7,   // KR=0.212; KB=0.087
	YCGCO                = 8,   // YCgCo or YCgCo-R
	BT2020_NCL           = 9,   // BT.2020-2 non-constant luminance (KR=0.2627; KB=0.0593)
	BT2020_CL            = 10,  // BT.2020-2 constant luminance
	SMPTE2085            = 11,  // Y'D'ZD'X
	CHROMA_DERIVED_NCL   = 12,  // Chromaticity-derived non-constant luminance
	CHROMA_DERIVED_CL    = 13,  // Chromaticity-derived constant luminance
	ICTCP                = 14,  // BT.2100 ICTCP
	IPT_C2               = 15,  // SMPTE IPT-PQ-C2
	YCGCO_RE             = 16,
	YCGCO_RO             = 17,
};

struct CicpInfo{
	ColorPrimaries primaries{ ColorPrimaries::RESERVED };
	TransferFunction transfer{ TransferFunction::RESERVED };
	MatrixCoefficients matrix{ MatrixCoefficients::IDENTITY };
	bool full_range{ true };
	
	CicpInfo() = default;
	CicpInfo( const CicpInfo& ) = default;
	CicpInfo( CicpInfo&& ) = default;
	CicpInfo& operator=( const CicpInfo& ) = default;
	CicpInfo& operator=( CicpInfo&& ) = default;
	
	bool is_valid() const { return primaries != ColorPrimaries::RESERVED && transfer != TransferFunction::RESERVED; }

	bool is_bt2020pq() const { return primaries == ColorPrimaries::BT2020 && (transfer == TransferFunction::SMPTE_ST_2084); }
};

class ColorProfile{
	private:
		cmsHPROFILE profile{ nullptr };
		ColorProfile( cmsHPROFILE profile ) : profile(profile) { }
	
	public:
		CicpInfo cicp;
		
	public:
		ColorProfile() { }
		ColorProfile( const ColorProfile& copy ) = delete;
		ColorProfile( ColorProfile&& other ){
			profile = other.profile;
			cicp = std::move( other.cicp );
			other.profile = nullptr;
		}
		ColorProfile& operator=( ColorProfile&& other ){
			cmsCloseProfile( profile );
			profile = other.profile;
			cicp = std::move( other.cicp );
			other.profile = nullptr;
			return *this;
		}
		~ColorProfile(){ cmsCloseProfile( profile ); }
		
		operator bool() const{ return profile; }
		
		static ColorProfile fromMem( const void* data, unsigned len )
			{ return { cmsOpenProfileFromMem( data, len ) }; }
		
		static ColorProfile fromFile( const char* path, const char* options )
			{ return { cmsOpenProfileFromFile( path, options ) }; }
		
		static ColorProfile sRgb(){ return { cmsCreate_sRGBProfile() }; }
		
		ColorTransform transformTo( const ColorProfile& to, unsigned in_format, unsigned out_format, unsigned intent, unsigned flags=0 ) const
			{ return ColorTransform( cmsCreateTransform( profile, in_format, to.profile, out_format, intent, flags ) ); }
};

class colorManager{
	private:
		std::vector<ColorProfile> monitors;
		std::vector<QScreen*> screens;
		
		ColorProfile p_srgb{ ColorProfile::sRgb() };
		
		
	public:
		colorManager();
		
		void doTransform( class QImage& img, const ColorProfile& in, QScreen* screen ) const;
};


#endif
