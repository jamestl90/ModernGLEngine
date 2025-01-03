#ifndef IMAGE_HELPERS_H
#define IMAGE_HELPERS_H

namespace JLEngine
{
	class RenderTarget;
	class ShaderProgram;

	class ImageHelpers
	{
	public:
		ImageHelpers();
		~ImageHelpers();

		// rasterizer method 
		static void Downsample(RenderTarget* input, RenderTarget* output, ShaderProgram* downsample);
		// TODO
		static void DownsampleCompute() {}

		static void BlurInPlaceCompute(RenderTarget* target, ShaderProgram* blurShader);
		
		static void CopyToDefault(RenderTarget* target, int defaultWidth, int defaultHeight, ShaderProgram* prog);

	protected:
		static unsigned int m_vaoID;
	};
}

#endif