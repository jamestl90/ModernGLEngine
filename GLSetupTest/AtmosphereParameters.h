#ifndef ATMOSPHERE_PARAMETERS_H
#define ATMOSPHERE_PARAMETERS_H

#include <glm/glm.hpp>

namespace JLEngine
{
	// Sources:
	// 1. https://cpp-rendering.io/sky-and-atmosphere-rendering/
	// 2. A Scalable and Production Ready Sky and Atmosphere Rendering Technique Sébastien Hillaire

	struct AtmosphereParams
	{
		// Sun 
		glm::vec3 solarIrradiance = glm::vec3(5.0f, 5.0f, 5.0f);	// amount of light received from all directions in the atmosphere
		float sunAngularRadius = 0.004675f;							// suns angular radius (from earth)
		alignas(16) glm::vec3 sunDir = glm::normalize(glm::vec3(0.0f, 0.3f, 1.0f));	// direction TO the sun (normalized, world space)
		float exposure = 10.0f;			// exposure val for hdr 

		// Earth
		float bottomRadiusKM = 6371.0f;			// planet radius
		float topRadiusKM = 6371.0f + 100.0f;	// distance from surface to top of atmosphere

		// should remove this, not currently used 
		alignas(16) glm::vec3 groundAlbedo = glm::vec3(0.1f, 0.1f, 0.1f);	// the colour of the ground 

		// Rayleigh Scattering
		alignas(16) glm::vec3 rayleighScatteringCoeff = glm::vec3(0.0058f, 0.0135f, 0.0331f);	// rayleigh scattering coeff
		float rayleighDensityHeightScale = 8.0f;			// density fallof scale by height

		// Mie Scattering
		alignas(16) glm::vec3 mieScatteringCoeff = glm::vec3(0.003996f);	// mi scattering coefficients 
		alignas(16) glm::vec3 mieExtinctionCoeff = glm::vec3(0.00444f);		// extinction fallof coefficients
		float mieDensityHeightScale = 1.2f;		// density falloff scale by height
		float miePhaseG = 0.76f;				// constant for henyey-greenstein phase function (cosine of scattering angle)

		// Absorption
		alignas(16) glm::vec3 absorptionExtinction = glm::vec3(0.00065f, 0.001881f, 0.000085f);	// absoption coefficient
		float absorptionDensityHeightScale = 15.0f;									// density falloff scale

		static AtmosphereParams NightTime()
		{
            AtmosphereParams nightParams; 
            nightParams.sunDir = glm::normalize(glm::vec3(0.0f, -0.5f, 1.0f));
            nightParams.solarIrradiance = glm::vec3(0.0001f, 0.0001f, 0.0001f); 
            nightParams.exposure = 20.0f; 
            nightParams.sunAngularRadius = 0.004675f;
            nightParams.bottomRadiusKM = 6371.0f;
            nightParams.topRadiusKM = 6371.0f + 100.0f;
            nightParams.groundAlbedo = glm::vec3(0.05f, 0.05f, 0.05f); 
            nightParams.rayleighScatteringCoeff = glm::vec3(0.0058f, 0.0135f, 0.0331f);
            nightParams.rayleighDensityHeightScale = 8.0f;
            nightParams.mieScatteringCoeff = glm::vec3(0.003996f);
            nightParams.mieExtinctionCoeff = glm::vec3(0.00444f);
            nightParams.mieDensityHeightScale = 1.2f;
            nightParams.miePhaseG = 0.76f; 
            nightParams.absorptionExtinction = glm::vec3(0.00065f, 0.001881f, 0.000085f);
            nightParams.absorptionDensityHeightScale = 15.0f;
            return nightParams;
		}

        static AtmosphereParams DayTime()
        {
            AtmosphereParams nightParams;
            nightParams.sunDir = glm::normalize(glm::vec3(0.0f, 0.3f, 1.0f));
            nightParams.solarIrradiance = glm::vec3(5.0f, 5.0f, 5.0f);
            nightParams.exposure = 10.0f;
            nightParams.sunAngularRadius = 0.004675f;
            nightParams.bottomRadiusKM = 6371.0f;
            nightParams.topRadiusKM = 6371.0f + 100.0f;
            nightParams.groundAlbedo = glm::vec3(0.05f, 0.05f, 0.05f);
            nightParams.rayleighScatteringCoeff = glm::vec3(0.0058f, 0.0135f, 0.0331f);
            nightParams.rayleighDensityHeightScale = 8.0f;
            nightParams.mieScatteringCoeff = glm::vec3(0.003996f);
            nightParams.mieExtinctionCoeff = glm::vec3(0.00444f);
            nightParams.mieDensityHeightScale = 1.2f;
            nightParams.miePhaseG = 0.76f;
            nightParams.absorptionExtinction = glm::vec3(0.00065f, 0.001881f, 0.000085f);
            nightParams.absorptionDensityHeightScale = 15.0f;
            return nightParams;
        }
	};
}

#endif
