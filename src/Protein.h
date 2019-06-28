#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <array>

namespace dynamol
{
	class Protein
	{
		struct Element
		{
			glm::vec3 color = glm::vec3(1.0, 1.0, 1.0);
			float radius = 1.0;
		};

	public:

		Protein();
		Protein(const std::string& filename);
		void load(const std::string& filename);
		const std::string & filename() const;

		const std::vector < std::vector<glm::vec4> > & atoms() const;
		const std::vector<Element> & elements() const;
		glm::vec3 minimumBounds() const;
		glm::vec3 maximumBounds() const;

		const std::vector<glm::vec4> & activeElementColorsRadiiPacked() const;
		const std::vector<glm::vec4> & activeResidueColorsPacked() const;
		const std::vector<glm::vec4> & activeChainColorsPacked() const;

		const std::vector<glm::uint> & activeElementIds() const;
		const std::vector<glm::uint> & activeResidueIds() const;
		const std::vector<glm::uint> & activeChainIds() const;

		const std::vector<float> & activeElementRadii() const;
		const std::vector<glm::vec3> & activeElementColors() const;
		const std::vector<glm::vec3> & activeResidueColors() const;
		const std::vector<glm::vec3> & activeChainColors() const;

		static const std::unordered_map<std::string, glm::uint> & elementIds();
		static const std::array<float, 116> & elementRadii();
		static const std::array<glm::vec3, 116> & elementColors();

		static const std::unordered_map<std::string, glm::uint> & residueIds();
		static const std::array<glm::vec3, 24> & residueColors();

		static const std::unordered_map<std::string, glm::uint> & chainIds();
		static const std::array<glm::vec3, 64> & chainColors();

	private:

		std::string m_filename;
		std::vector<std::vector<glm::vec4> > m_atoms;

		std::array<glm::uint, 116> m_elementIdMap;
		std::array<glm::uint, 24> m_residueIdMap;
		std::array<glm::uint, 64> m_chainIdMap;

		std::vector<glm::uint> m_activeElementIds;
		std::vector<glm::uint> m_activeResidueIds;
		std::vector<glm::uint> m_activeChainIds;

		std::vector<float> m_activeElementRadii;
		std::vector<glm::vec3> m_activeElementColors;
		std::vector<glm::vec3> m_activeResidueColors;
		std::vector<glm::vec3> m_activeChainColors;

		std::vector<glm::vec4> m_activeElementColorsRadiiPacked;
		std::vector<glm::vec4> m_activeResidueColorsPacked;
		std::vector<glm::vec4> m_activeChainColorsPacked;

		glm::vec3 m_minimumBounds = glm::vec3(0.0);
		glm::vec3 m_maximumBounds = glm::vec3(0.0);
	};
}