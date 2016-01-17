#pragma once

#include "GlobalAppState.h"
#include "TimingLogDepthSensing.h"

#include "MatrixConversion.h"
#include "cuda_SimpleMatrixUtil.h"
#include "DepthCameraUtil.h"
#include "RayCastSDFUtil.h"

#include "DX11RayIntervalSplatting.h"

class CUDARayCastSDF
{
public:
	CUDARayCastSDF(const RayCastParams& params) {
		create(params);
	}

	~CUDARayCastSDF(void) {
		destroy();
	}

	static RayCastParams parametersFromGlobalAppState(const GlobalAppState& gas, const mat4f& intrinsics, const mat4f& intrinsicsInv) {
		mat4f rayCastIntrinsics = intrinsics;
		if (gas.s_rayCastWidth != gas.s_integrationWidth || gas.s_rayCastHeight != gas.s_integrationHeight) {
			const float scaleWidthDepth = (float)gas.s_rayCastWidth / (float)gas.s_integrationWidth;
			const float scaleHeightDepth = (float)gas.s_rayCastHeight / (float)gas.s_integrationHeight;
			// adapt intrinsics
			rayCastIntrinsics._m00 *= scaleWidthDepth;  rayCastIntrinsics._m02 *= scaleWidthDepth;
			rayCastIntrinsics._m11 *= scaleHeightDepth; rayCastIntrinsics._m12 *= scaleHeightDepth;
		}

		RayCastParams params;
		params.m_width = gas.s_rayCastWidth;			
		params.m_height = gas.s_rayCastHeight;			
		params.fx = rayCastIntrinsics(0, 0);
		params.fy = rayCastIntrinsics(1, 1);
		params.mx = rayCastIntrinsics(0, 2);
		params.my = rayCastIntrinsics(1, 2);
		params.m_minDepth = gas.s_renderDepthMin;
		params.m_maxDepth = gas.s_renderDepthMax;
		params.m_rayIncrement = gas.s_SDFRayIncrementFactor * gas.s_SDFTruncation;
		params.m_thresSampleDist = gas.s_SDFRayThresSampleDistFactor * params.m_rayIncrement;
		params.m_thresDist = gas.s_SDFRayThresDistFactor * params.m_rayIncrement;
		params.m_useGradients = gas.s_SDFUseGradients;

		params.m_maxNumVertices = gas.s_hashNumSDFBlocks * 6;

		return params;
	}

	void render(const HashData& hashData, const HashParams& hashParams, const mat4f& lastRigidTransform);

	const RayCastData& getRayCastData(void) {
		return m_data;
	}
	const RayCastParams& getRayCastParams() const {
		return m_params;
	}

	mat4f getIntrinsicsInv() const { return m_rayCastIntrinsicsInverse; }
	mat4f getIntrinsics() const { return m_rayCastIntrinsics; }

	//! the actual raycast calls the gpu update
	void updateRayCastMinMax(float depthMin, float depthMax) {
		m_params.m_minDepth = depthMin;
		m_params.m_maxDepth = depthMax;
	}

	// debugging
	void convertToCameraSpace();

private:

	void create(const RayCastParams& params);
	void destroy(void);

	void rayIntervalSplatting(const HashData& hashData, const HashParams& hashParams, const mat4f& lastRigidTransform); // rasterize

	RayCastParams m_params;
	RayCastData m_data;
	mat4f m_rayCastIntrinsics;
	mat4f m_rayCastIntrinsicsInverse;

	DX11RayIntervalSplatting m_rayIntervalSplatting;

	static Timer m_timer;
};

