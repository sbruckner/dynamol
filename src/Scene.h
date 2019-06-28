#pragma once

#include <memory>

namespace dynamol
{
	class Protein;

	class Scene
	{
	public:
		Scene();
		Protein* protein();

	private:
		std::unique_ptr<Protein> m_protein;
	};


}