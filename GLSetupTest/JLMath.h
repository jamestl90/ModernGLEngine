#ifndef JL_MATH_H
#define JL_MATH_H

#include <vector>
#include <glm/glm.hpp>

namespace JLEngine
{
	class Math
	{
	public:
		static std::vector<float> CalculateGaussianWeights(int radius)
		{
			/*
			* G(x) = e^(-x^2 / 2 * sigma^2)
			*/
			float sigma = radius / 3.0f;
			float twoSigmaSq = 2.0f * sigma * sigma;
			std::vector<float> weights(radius * 2 + 1); // +1 because of the center pixel
			float totalWeight = 0.0f;

			for (int i = -radius; i <= radius; i++)
			{
				float weight = glm::exp(-(i * i) / twoSigmaSq);
				weights[i + radius] = weight; // offset by radius because we're starting at -radius
				totalWeight += weight;
			}

			// normalize weights
			for (size_t i = 0; i < weights.size(); i++)
			{
				weights[i] /= totalWeight;
			}

			return weights;
		}
	};
}

#endif