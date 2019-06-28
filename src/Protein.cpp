#include "Protein.h"

#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <iterator>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <array>
#include <algorithm> 
#include <cctype>
#include <locale>
#include <globjects/globjects.h>
#include <globjects/logging.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace dynamol;
using namespace glm;

// trim from start (in place)
static inline void ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

// trim from end (in place)
static inline void rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s)
{
	ltrim(s);
	rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s)
{
	ltrim(s);
	return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s)
{
	rtrim(s);
	return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s)
{
	trim(s);
	return s;
}

Protein::Protein()
{

}

Protein::Protein(const std::string& filename)
{
	load(filename);
}

void Protein::load(const std::string& filename)
{
	globjects::debug() << "Loading file " << filename << " ...";
	std::ifstream file(filename);

	if (!file.is_open())
	{
		globjects::critical() << "Could not open file " << filename << "!";
	}

	m_filename = filename;

	m_atoms.clear();
	m_minimumBounds = vec3(std::numeric_limits<float>::max());
	m_maximumBounds = vec3(-std::numeric_limits<float>::max());

	m_elementIdMap.fill(0);
	m_residueIdMap.fill(0);
	m_chainIdMap.fill(0);

	m_activeElementIds.clear();
	m_activeElementIds.push_back(0);

	m_activeResidueIds.clear();
	m_activeResidueIds.push_back(0);

	m_activeChainIds.clear();
	m_activeChainIds.push_back(0);

	std::string str;
	uint elementCount = 1;

	std::vector<vec4> atoms;

	while (std::getline(file, str))
	{
		//std::vector<std::string> tokens;
		//std::istringstream buffer(str);
		//std::copy(std::istream_iterator<std::string>(buffer), std::istream_iterator<std::string>(), std::back_inserter(tokens));

		std::string recordName = trim_copy(str.substr(0, 6));

		if (recordName == "END")
		{
			m_atoms.push_back(atoms);
			atoms.clear();
		}
		else if (recordName == "ATOM" || recordName == "HETATM")
		{
			float x = float(std::atof(trim_copy(str.substr(30, 8)).c_str()));
			float y = float(std::atof(trim_copy(str.substr(38, 8)).c_str()));
			float z = float(std::atof(trim_copy(str.substr(46, 8)).c_str()));


			std::string residueName = trim_copy(str.substr(17, 3));
			std::string chainName = trim_copy(str.substr(21, 1));
			std::string elementName = trim_copy(str.substr(76, 2));

			uint elementId = 0;
			uint elementIndex = 0;
			
			auto ei = elementIds().find(elementName);

			if (ei != elementIds().end())
				elementId = ei->second;

			elementIndex = m_elementIdMap[elementId];

			if (elementIndex == 0)
			{
				elementIndex = uint(m_activeElementIds.size());
				m_activeElementIds.push_back(elementId);				
				m_elementIdMap[elementId] = elementIndex;
			}

			uint residueId = 0;
			uint residueIndex = 0;

			auto ri = residueIds().find(residueName);

			if (ri != residueIds().end())
				residueId = ri->second;

			residueIndex = m_residueIdMap[residueId];

			if (residueIndex == 0)
			{
				residueIndex = uint(m_activeResidueIds.size());
				m_activeResidueIds.push_back(residueId);
				m_residueIdMap[residueId] = residueIndex;
			}

			uint chainId = 0;
			uint chainIndex = 0;

			auto ci = chainIds().find(chainName);

			if (ci != chainIds().end())
				chainId = ci->second;

			chainIndex = m_chainIdMap[chainId];

			if (chainIndex == 0)
			{
				chainIndex = uint(m_activeChainIds.size());
				m_activeChainIds.push_back(chainId);
				m_chainIdMap[chainId] = chainIndex;
			}

			uint atomAttributes = elementIndex | (residueIndex << 8) | (chainIndex << 16);
			vec4 atom(x, y, z, uintBitsToFloat(atomAttributes));
			atoms.push_back(atom);

			m_minimumBounds = min(m_minimumBounds, vec3(atom));
			m_maximumBounds = max(m_maximumBounds, vec3(atom));
		}
	}

	for (auto id : m_activeElementIds)
	{
		m_activeElementColors.push_back(elementColors()[id]);
		m_activeElementRadii.push_back(elementRadii()[id]);
		m_activeElementColorsRadiiPacked.push_back(vec4(elementColors()[id],elementRadii()[id]));
	}

	for (auto id : m_activeResidueIds)
	{
		m_activeResidueColors.push_back(residueColors()[id]);
		m_activeResidueColorsPacked.push_back(vec4(residueColors()[id],1.0f));
	}

	for (auto id : m_activeChainIds)
	{
		m_activeChainColors.push_back(chainColors()[id]);
		m_activeChainColorsPacked.push_back(vec4(chainColors()[id], 1.0f));
	}


	for (uint i = 0; i < m_atoms.size(); i++)
	{
		globjects::debug() << "  Timestep " << i << ": " << uint(m_atoms[i].size()) << " atoms";
	}

	globjects::debug() << uint(m_atoms.size()) << " timesteps loaded." << std::endl;
}

const std::string & Protein::filename() const
{
	return m_filename;
}

const std::vector< std::vector<glm::vec4> > & Protein::atoms() const
{
	return m_atoms;
}

vec3 Protein::minimumBounds() const
{
	return m_minimumBounds;
}

vec3 Protein::maximumBounds() const
{
	return m_maximumBounds;
}

const std::vector<glm::vec4>& Protein::activeElementColorsRadiiPacked() const
{
	return m_activeElementColorsRadiiPacked;
}

const std::vector<glm::vec4>& dynamol::Protein::activeResidueColorsPacked() const
{
	return m_activeResidueColorsPacked;
}

const std::vector<glm::vec4>& Protein::activeChainColorsPacked() const
{
	return m_activeChainColorsPacked;
}

const std::vector<uint>& Protein::activeElementIds() const
{
	return m_activeElementIds;
}

const std::vector<uint>& dynamol::Protein::activeResidueIds() const
{
	return m_activeResidueIds;
}

const std::vector<uint>& dynamol::Protein::activeChainIds() const
{
	return m_activeChainIds;
}

const std::vector<float>& dynamol::Protein::activeElementRadii() const
{
	return m_activeElementRadii;
}

const std::vector<glm::vec3>& dynamol::Protein::activeElementColors() const
{
	return m_activeElementColors;
}

const std::vector<glm::vec3>& dynamol::Protein::activeResidueColors() const
{
	return m_activeResidueColors;
}

const std::vector<glm::vec3>& dynamol::Protein::activeChainColors() const
{
	return m_activeChainColors;
}

const std::unordered_map<std::string, uint>& dynamol::Protein::elementIds()
{
	static const std::unordered_map<std::string, uint> elementIds = {
		{"H",1},
		{"He",2},
		{"Li",3},
		{"Be",4},
		{"B",5},
		{"C",6},
		{"N",7},
		{"O",8},
		{"F",9},
		{"Ne",10},
		{"Na",11},
		{"Mg",12},
		{"Al",13},
		{"Si",14},
		{"P",15},
		{"S",16},
		{"Cl",17},
		{"Ar",18},
		{"K",19},
		{"Ca",20},
		{"Sc",21},
		{"Ti",22},
		{"V",23},
		{"Cr",24},
		{"Mn",25},
		{"Fe",26},
		{"Co",27},
		{"Ni",28},
		{"Cu",29},
		{"Zn",30},
		{"Ga",31},
		{"Ge",32},
		{"As",33},
		{"Se",34},
		{"Br",35},
		{"Kr",36},
		{"Rb",37},
		{"Sr",38},
		{"Y",39},
		{"Zr",40},
		{"Nb",41},
		{"Mo",42},
		{"Tc",43},
		{"Ru",44},
		{"Rh",45},
		{"Pd",46},
		{"Ag",47},
		{"Cd",48},
		{"In",49},
		{"Sn",50},
		{"Sb",51},
		{"Te",52},
		{"I",53},
		{"Xe",54},
		{"Cs",55},
		{"Ba",56},
		{"La",57},
		{"Ce",58},
		{"Pr",59},
		{"Nd",60},
		{"Pm",61},
		{"Sm",62},
		{"Eu",63},
		{"Gd",64},
		{"Tb",65},
		{"Dy",66},
		{"Ho",67},
		{"Er",68},
		{"Tm",69},
		{"Yb",70},
		{"Lu",71},
		{"Hf",72},
		{"Ta",73},
		{"W",74},
		{"Re",75},
		{"Os",76},
		{"Ir",77},
		{"Pt",78},
		{"Au",79},
		{"Hg",80},
		{"Tl",81},
		{"Pb",82},
		{"Bi",83},
		{"Po",84},
		{"At",85},
		{"Rn",86},
		{"Fr",87},
		{"Ra",88},
		{"Ac",89},
		{"Th",90},
		{"Pa",91},
		{"U",92},
		{"Np",93},
		{"Pu",94},
		{"Am",95},
		{"Cm",96},
		{"Bk",97},
		{"Cf",98},
		{"Es",99},
		{"Fm",100},
		{"Md",101},
		{"No",102},
		{"Lr",103},
		{"Rf",104},
		{"Db",105},
		{"Sg",106},
		{"Bh",107},
		{"Hs",108},
		{"Mt",109},
		{"Ds",110},
		{"Rg",111},
		{"Cn",112},
		{"H (WAT)",113},
		{"O (WAT)",114},
		{"D",115 }
	};

	return elementIds;
}

const std::array<float, 116>& Protein::elementRadii()
{
	static const std::array<float,116> elementRadii = {
		1.5f, // 0 - default
		/*<vdw id = "1" radius = "*/1.200f,
		/*<vdw id = "2" radius = "*/1.400f,
		/*<vdw id = "3" radius = "*/1.820f,
		/*<vdw id = "4" radius = "*/1.530f,
		/*<vdw id = "5" radius = "*/1.920f,
		/*<vdw id = "6" radius = "*/1.700f,
		/*<vdw id = "7" radius = "*/1.550f,
		/*<vdw id = "8" radius = "*/1.520f,
		/*<vdw id = "9" radius = "*/1.470f,
		/*<vdw id = "10" radius = "*/1.540f,
		/*<vdw id = "11" radius = "*/2.270f,
		/*<vdw id = "12" radius = "*/1.730f,
		/*<vdw id = "13" radius = "*/1.840f,
		/*<vdw id = "14" radius = "*/2.100f,
		/*<vdw id = "15" radius = "*/1.800f,
		/*<vdw id = "16" radius = "*/1.800f,
		/*<vdw id = "17" radius = "*/1.750f,
		/*<vdw id = "18" radius = "*/1.880f,
		/*<vdw id = "19" radius = "*/2.750f,
		/*<vdw id = "20" radius = "*/2.310f,
		/*<vdw id = "21" radius = "*/2.110f,
		/*<vdw id = "22" radius = "*/2.000f,
		/*<vdw id = "23" radius = "*/2.000f,
		/*<vdw id = "24" radius = "*/2.000f,
		/*<vdw id = "25" radius = "*/2.000f,
		/*<vdw id = "26" radius = "*/2.000f,
		/*<vdw id = "27" radius = "*/2.000f,
		/*<vdw id = "28" radius = "*/1.630f,
		/*<vdw id = "29" radius = "*/1.400f,
		/*<vdw id = "30" radius = "*/1.390f,
		/*<vdw id = "31" radius = "*/1.870f,
		/*<vdw id = "32" radius = "*/2.110f,
		/*<vdw id = "33" radius = "*/1.850f,
		/*<vdw id = "34" radius = "*/1.900f,
		/*<vdw id = "35" radius = "*/1.850f,
		/*<vdw id = "36" radius = "*/2.020f,
		/*<vdw id = "37" radius = "*/3.030f,
		/*<vdw id = "38" radius = "*/2.490f,
		/*<vdw id = "39" radius = "*/2.000f,
		/*<vdw id = "40" radius = "*/2.000f,
		/*<vdw id = "41" radius = "*/2.000f,
		/*<vdw id = "42" radius = "*/2.000f,
		/*<vdw id = "43" radius = "*/2.000f,
		/*<vdw id = "44" radius = "*/2.000f,
		/*<vdw id = "45" radius = "*/2.000f,
		/*<vdw id = "46" radius = "*/1.630f,
		/*<vdw id = "47" radius = "*/1.720f,
		/*<vdw id = "48" radius = "*/1.580f,
		/*<vdw id = "49" radius = "*/1.930f,
		/*<vdw id = "50" radius = "*/2.170f,
		/*<vdw id = "51" radius = "*/2.060f,
		/*<vdw id = "52" radius = "*/2.060f,
		/*<vdw id = "53" radius = "*/1.980f,
		/*<vdw id = "54" radius = "*/2.160f,
		/*<vdw id = "55" radius = "*/3.430f,
		/*<vdw id = "56" radius = "*/2.680f,
		/*<vdw id = "57" radius = "*/2.000f,
		/*<vdw id = "58" radius = "*/2.000f,
		/*<vdw id = "59" radius = "*/2.000f,
		/*<vdw id = "60" radius = "*/2.000f,
		/*<vdw id = "61" radius = "*/2.000f,
		/*<vdw id = "62" radius = "*/2.000f,
		/*<vdw id = "63" radius = "*/2.000f,
		/*<vdw id = "64" radius = "*/2.000f,
		/*<vdw id = "65" radius = "*/2.000f,
		/*<vdw id = "66" radius = "*/2.000f,
		/*<vdw id = "67" radius = "*/2.000f,
		/*<vdw id = "68" radius = "*/2.000f,
		/*<vdw id = "69" radius = "*/2.000f,
		/*<vdw id = "70" radius = "*/2.000f,
		/*<vdw id = "71" radius = "*/2.000f,
		/*<vdw id = "72" radius = "*/2.000f,
		/*<vdw id = "73" radius = "*/2.000f,
		/*<vdw id = "74" radius = "*/2.000f,
		/*<vdw id = "75" radius = "*/2.000f,
		/*<vdw id = "76" radius = "*/2.000f,
		/*<vdw id = "77" radius = "*/2.000f,
		/*<vdw id = "78" radius = "*/1.750f,
		/*<vdw id = "79" radius = "*/1.660f,
		/*<vdw id = "80" radius = "*/1.550f,
		/*<vdw id = "81" radius = "*/1.960f,
		/*<vdw id = "82" radius = "*/2.020f,
		/*<vdw id = "83" radius = "*/2.070f,
		/*<vdw id = "84" radius = "*/1.970f,
		/*<vdw id = "85" radius = "*/2.020f,
		/*<vdw id = "86" radius = "*/2.200f,
		/*<vdw id = "87" radius = "*/3.480f,
		/*<vdw id = "88" radius = "*/2.830f,
		/*<vdw id = "89" radius = "*/2.000f,
		/*<vdw id = "90" radius = "*/2.000f,
		/*<vdw id = "91" radius = "*/2.000f,
		/*<vdw id = "92" radius = "*/1.860f,
		/*<vdw id = "93" radius = "*/2.000f,
		/*<vdw id = "94" radius = "*/2.000f,
		/*<vdw id = "95" radius = "*/2.000f,
		/*<vdw id = "96" radius = "*/2.000f,
		/*<vdw id = "97" radius = "*/2.000f,
		/*<vdw id = "98" radius = "*/2.000f,
		/*<vdw id = "99" radius = "*/2.000f,
		/*<vdw id = "100" radius = "*/2.000f,
		/*<vdw id = "101" radius = "*/2.000f,
		/*<vdw id = "102" radius = "*/2.000f,
		/*<vdw id = "103" radius = "*/2.000f,
		/*<vdw id = "104" radius = "*/2.000f,
		/*<vdw id = "105" radius = "*/2.000f,
		/*<vdw id = "106" radius = "*/2.000f,
		/*<vdw id = "107" radius = "*/2.000f,
		/*<vdw id = "108" radius = "*/2.000f,
		/*<vdw id = "109" radius = "*/2.000f,
		/*<vdw id = "110" radius = "*/2.000f,
		/*<vdw id = "111" radius = "*/2.000f,
		/*<vdw id = "112" radius = "*/2.000f,
		/*<vdw id = "113" radius = "*/1.200f,
		/*<vdw id = "114" radius = "*/1.520f,
		/*<vdw id = "115" radius = "*/1.200f
	};

	return elementRadii;
}

const std::array<vec3, 116>& Protein::elementColors()
{
	// source: http://jmol.sourceforge.net/jscolors/
	static const std::array<vec3,116> elementColors = {
		vec3(1.0f,1.0f,1.0f),
		vec3(255, 255, 255) / 255.0f,
		vec3(217, 255, 255) / 255.0f,
		vec3(204, 128, 255) / 255.0f,
		vec3(194, 255, 0) / 255.0f,
		vec3(255, 181, 181) / 255.0f,
		vec3(144, 144, 144) / 255.0f,
		vec3(48, 80, 248) / 255.0f,
		vec3(255, 13, 13) / 255.0f,
		vec3(144, 224, 80) / 255.0f,
		vec3(179, 227, 245) / 255.0f,
		vec3(171, 92, 242) / 255.0f,
		vec3(138, 255, 0) / 255.0f,
		vec3(191, 166, 166) / 255.0f,
		vec3(240, 200, 160) / 255.0f,
		vec3(255, 128, 0) / 255.0f,
		vec3(255, 255, 48) / 255.0f,
		vec3(31, 240, 31) / 255.0f,
		vec3(128, 209, 227) / 255.0f,
		vec3(143, 64, 212) / 255.0f,
		vec3(61, 255, 0) / 255.0f,
		vec3(230, 230, 230) / 255.0f,
		vec3(191, 194, 199) / 255.0f,
		vec3(166, 166, 171) / 255.0f,
		vec3(138, 153, 199) / 255.0f,
		vec3(156, 122, 199) / 255.0f,
		vec3(224, 102, 51) / 255.0f,
		vec3(240, 144, 160) / 255.0f,
		vec3(80, 208, 80) / 255.0f,
		vec3(200, 128, 51) / 255.0f,
		vec3(125, 128, 176) / 255.0f,
		vec3(194, 143, 143) / 255.0f,
		vec3(102, 143, 143) / 255.0f,
		vec3(189, 128, 227) / 255.0f,
		vec3(255, 161, 0) / 255.0f,
		vec3(166, 41, 41) / 255.0f,
		vec3(92, 184, 209) / 255.0f,
		vec3(112, 46, 176) / 255.0f,
		vec3(0, 255, 0) / 255.0f,
		vec3(148, 255, 255) / 255.0f,
		vec3(148, 224, 224) / 255.0f,
		vec3(115, 194, 201) / 255.0f,
		vec3(84, 181, 181) / 255.0f,
		vec3(59, 158, 158) / 255.0f,
		vec3(36, 143, 143) / 255.0f,
		vec3(10, 125, 140) / 255.0f,
		vec3(0, 105, 133) / 255.0f,
		vec3(192, 192, 192) / 255.0f,
		vec3(255, 217, 143) / 255.0f,
		vec3(166, 117, 115) / 255.0f,
		vec3(102, 128, 128) / 255.0f,
		vec3(158, 99, 181) / 255.0f,
		vec3(212, 122, 0) / 255.0f,
		vec3(148, 0, 148) / 255.0f,
		vec3(66, 158, 176) / 255.0f,
		vec3(87, 23, 143) / 255.0f,
		vec3(0, 201, 0) / 255.0f,
		vec3(112, 212, 255) / 255.0f,
		vec3(255, 255, 199) / 255.0f,
		vec3(217, 255, 199) / 255.0f,
		vec3(199, 255, 199) / 255.0f,
		vec3(163, 255, 199) / 255.0f,
		vec3(143, 255, 199) / 255.0f,
		vec3(97, 255, 199) / 255.0f,
		vec3(69, 255, 199) / 255.0f,
		vec3(48, 255, 199) / 255.0f,
		vec3(31, 255, 199) / 255.0f,
		vec3(0, 255, 156) / 255.0f,
		vec3(0, 230, 117) / 255.0f,
		vec3(0, 212, 82) / 255.0f,
		vec3(0, 191, 56) / 255.0f,
		vec3(0, 171, 36) / 255.0f,
		vec3(77, 194, 255) / 255.0f,
		vec3(77, 166, 255) / 255.0f,
		vec3(33, 148, 214) / 255.0f,
		vec3(38, 125, 171) / 255.0f,
		vec3(38, 102, 150) / 255.0f,
		vec3(23, 84, 135) / 255.0f,
		vec3(208, 208, 224) / 255.0f,
		vec3(255, 209, 35) / 255.0f,
		vec3(184, 184, 208) / 255.0f,
		vec3(166, 84, 77) / 255.0f,
		vec3(87, 89, 97) / 255.0f,
		vec3(158, 79, 181) / 255.0f,
		vec3(171, 92, 0) / 255.0f,
		vec3(117, 79, 69) / 255.0f,
		vec3(66, 130, 150) / 255.0f,
		vec3(66, 0, 102) / 255.0f,
		vec3(0, 125, 0) / 255.0f,
		vec3(112, 171, 250) / 255.0f,
		vec3(0, 186, 255) / 255.0f,
		vec3(0, 161, 255) / 255.0f,
		vec3(0, 143, 255) / 255.0f,
		vec3(0, 128, 255) / 255.0f,
		vec3(0, 107, 255) / 255.0f,
		vec3(84, 92, 242) / 255.0f,
		vec3(120, 92, 227) / 255.0f,
		vec3(138, 79, 227) / 255.0f,
		vec3(161, 54, 212) / 255.0f,
		vec3(179, 31, 212) / 255.0f,
		vec3(179, 31, 186) / 255.0f,
		vec3(179, 13, 166) / 255.0f,
		vec3(189, 13, 135) / 255.0f,
		vec3(199, 0, 102) / 255.0f,
		vec3(204, 0, 89) / 255.0f,
		vec3(209, 0, 79) / 255.0f,
		vec3(217, 0, 69) / 255.0f,
		vec3(224, 0, 56) / 255.0f,
		vec3(230, 0, 46) / 255.0f,
		vec3(235, 0, 38) / 255.0f,
		vec3(1.0f, 1.0f, 1.0f),
		vec3(1.0f, 1.0f, 1.0f),
		vec3(1.0f, 1.0f, 1.0f),
		vec3(1.0f, 1.0f, 1.0f),
		vec3(1.0f, 1.0f, 1.0f),
	};

	return elementColors;
}

const std::unordered_map<std::string, uint>& Protein::residueIds()
{
	static const std::unordered_map<std::string, uint> residueColors = {
		{"ALA",1},
		{"ARG",2},
		{"ASN",3},
		{"ASP",4},
		{"CYS",5},
		{"GLN",6},
		{"GLU",7},
		{"GLY",8},
		{"HIS",9},
		{"ILE",10},
		{"LEU",11},
		{"LYS",12},
		{"MET",13},
		{"PHE",14},
		{"PRO",15},
		{"SER",16},
		{"THR",17},
		{"TRP",18},
		{"TYR",19},
		{"VAL",20},
		{"ASX",21},
		{"GLX",22},
		{"other",23 }
	};
	
	return residueColors;
}

const std::array<vec3, 24>& Protein::residueColors()
{
	static const std::array<vec3, 24> residueColors = {
		/* default */ vec3(1.0f,1.0f,1.0f),
		/* Ala */ vec3(200, 200, 200) / 255.0f,
		/* Arg */ vec3(20, 90, 255) / 255.0f,
		/* Asn */ vec3(0, 220, 220) / 255.0f,
		/* Asp */ vec3(230, 10, 10) / 255.0f,
		/* Cys */ vec3(230, 230, 0) / 255.0f,
		/* Gln */ vec3(0, 220, 220) / 255.0f,
		/* Glu */ vec3(230, 10, 10) / 255.0f,
		/* Gly */ vec3(235, 235, 235) / 255.0f,
		/* His */ vec3(130, 130, 210) / 255.0f,
		/* Ile */ vec3(15, 130, 15) / 255.0f,
		/* Leu */ vec3(15, 130, 15) / 255.0f,
		/* Lys */ vec3(20, 90, 255) / 255.0f,
		/* Met */ vec3(230, 230, 0) / 255.0f,
		/* Phe */ vec3(50, 50, 170) / 255.0f,
		/* Pro */ vec3(220, 150, 130) / 255.0f,
		/* Ser */ vec3(250, 150, 0) / 255.0f,
		/* Thr */ vec3(250, 150, 0) / 255.0f,
		/* Trp */ vec3(180, 90, 180) / 255.0f,
		/* Tyr */ vec3(50, 50, 170) / 255.0f,
		/* Val */ vec3(15, 130, 15) / 255.0f,
		/* Asx */ vec3(255, 105, 180) / 255.0f,
		/* Glx */ vec3(255, 105, 180) / 255.0f,
		/* other */ vec3(190, 160, 110) / 255.0f
	};

	return residueColors;
}

const std::unordered_map<std::string, uint>& Protein::chainIds()
{
	static const std::unordered_map<std::string, uint> chainIds = {
		{"A",1},
		{"a",2},
		{"B",3},
		{"b",4},
		{"C",5},
		{"c",6},
		{"D",7},
		{"d",8},
		{"E",9},
		{"e",10},
		{"F",11},
		{"f",12},
		{"G",13},
		{"g",14},
		{"H",15},
		{"h",16},
		{"I",17},
		{"i",18},
		{"J",19},
		{"j",20},
		{"K",21},
		{"k",22},
		{"L",23},
		{"l",24},
		{"M",25},
		{"m",26},
		{"N",27},
		{"n",28},
		{"O",29},
		{"o",30},
		{"P",31},
		{"p",32},
		{"0",33},
		{"Q",34},
		{"q",35},
		{"1",36},
		{"R",37},
		{"r",38},
		{"2",39},
		{"S",40},
		{"s",41},
		{"3",42},
		{"T",43},
		{"t",44},
		{"4",45},
		{"U",46},
		{"u",47},
		{"5",48},
		{"V",49},
		{"v",50},
		{"6",51},
		{"W",52},
		{"w",53},
		{"7",54},
		{"X",55},
		{"x",56},
		{"8",57},
		{"Y",58},
		{"y",59},
		{"9",60},
		{"Z",61},
		{"z",62},
		{"none",63}
	};

	return chainIds;
}

const std::array<vec3, 64>& Protein::chainColors()
{
	static const std::array<vec3, 64> chainColors = {
		/* default */ vec3(1.0f,1.0f,1.0f),
		/* A */ vec3(192,208,255) / 255.0f,
		/* a */ vec3(192,208,255) / 255.0f,
		/* B */ vec3(176,255,176) / 255.0f,
		/* b */ vec3(176,255,176) / 255.0f,
		/* C */ vec3(255,192,200) / 255.0f,
		/* c */ vec3(255,192,200) / 255.0f,
		/* D */ vec3(255,255,128) / 255.0f,
		/* d */ vec3(255,255,128) / 255.0f,
		/* E */ vec3(255,192,255) / 255.0f,
		/* e */ vec3(255,192,255) / 255.0f,
		/* F */ vec3(176,240,240) / 255.0f,
		/* f */ vec3(176,240,240) / 255.0f,
		/* G */ vec3(255,208,112) / 255.0f,
		/* g */ vec3(255,208,112) / 255.0f,
		/* H */ vec3(240,128,128) / 255.0f,
		/* h */ vec3(240,128,128) / 255.0f,
		/* I */ vec3(245,222,179) / 255.0f,
		/* i */ vec3(245,222,179) / 255.0f,
		/* J */ vec3(0,191,255) / 255.0f,
		/* j */ vec3(0,191,255) / 255.0f,
		/* K */ vec3(205,92,92) / 255.0f,
		/* k */ vec3(205,92,92) / 255.0f,
		/* L */ vec3(102,205,170) / 255.0f,
		/* l */ vec3(102,205,170) / 255.0f,
		/* M */ vec3(154,205,50) / 255.0f,
		/* m */ vec3(154,205,50) / 255.0f,
		/* N */ vec3(238,130,238) / 255.0f,
		/* n */ vec3(238,130,238) / 255.0f,
		/* O */ vec3(0,206,209) / 255.0f,
		/* o */ vec3(0,206,209) / 255.0f,
		/* P */ vec3(0,255,127) / 255.0f,
		/* p */ vec3(0,255,127) / 255.0f,
		/* 0 */ vec3(0,255,127) / 255.0f,
		/* Q */ vec3(60,179,113) / 255.0f,
		/* q */ vec3(60,179,113) / 255.0f,
		/* 1 */ vec3(60,179,113) / 255.0f,
		/* R */ vec3(0,0,139) / 255.0f,
		/* r */ vec3(0,0,139) / 255.0f,
		/* 2 */ vec3(0,0,139) / 255.0f,
		/* S */ vec3(189,183,107) / 255.0f,
		/* s */ vec3(189,183,107) / 255.0f,
		/* 3 */ vec3(189,183,107) / 255.0f,
		/* T */ vec3(0,100,0) / 255.0f,
		/* t */ vec3(0,100,0) / 255.0f,
		/* 4 */ vec3(0,100,0) / 255.0f,
		/* U */ vec3(128,0,0) / 255.0f,
		/* u */ vec3(128,0,0) / 255.0f,
		/* 5 */ vec3(128,0,0) / 255.0f,
		/* V */ vec3(128,128,0) / 255.0f,
		/* v */ vec3(128,128,0) / 255.0f,
		/* 6 */ vec3(128,128,0) / 255.0f,
		/* W */ vec3(128,0,128) / 255.0f,
		/* w */ vec3(128,0,128) / 255.0f,
		/* 7 */ vec3(128,0,128) / 255.0f,
		/* X */ vec3(0,128,128) / 255.0f,
		/* x */ vec3(0,128,128) / 255.0f,
		/* 8 */ vec3(0,128,128) / 255.0f,
		/* Y */ vec3(184,134,11) / 255.0f,
		/* y */ vec3(184,134,11) / 255.0f,
		/* 9 */ vec3(184,134,11) / 255.0f,
		/* Z */ vec3(178,34,34) / 255.0f,
		/* z */ vec3(178,34,34) / 255.0f,
		/* none */ vec3(255,255,255) / 255.0f
	};

	return chainColors;
}
