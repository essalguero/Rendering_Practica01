//#include <tbb/tbb.h>
#include <stdio.h>
#include <math.h>
#include <gmtl/gmtl.h>
#include <imageio.h>

#include <world.h>

#include <parsers/ass_parser.h>

#include <reporter.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <algorithm>

#include <main.h>


#include <standard.h>

#include "lights\pointlight.h"

#include <cmath>

#include <iostream>

int g_RenderMaxDepth = 12;

extern int g_pixel_samples;

const float AMBIENT_INTENSITY = 0.005f;

World* ReadFromFile(const char* filename)
{
	World* world;
	if (!ReadAssFile(filename, world))
	{
		Error("Could not read file: %s", filename);
		return NULL;
	}
	return world;
}



gmtl::Rayf generateRay(gmtl::Point3f initialPosition, gmtl::Point3f finalPosition) {
	// ray generated
	gmtl::Rayf ray;

	// set the direction of the ray

	// Direction to the light being taken into account
	gmtl::Vec3f direction(finalPosition - initialPosition);
	normalize(direction);
	ray.setOrigin(initialPosition + direction * 0.01f);
	//ray.setOrigin(initialPosition);
	ray.setDir(direction);

	return ray;
}


Spectrum calculateDiffuseComponent(World* world, PointLight* light, IntersectInfo& intersectInfo)
{
	Standard* material = (Standard *)intersectInfo.material;
	Spectrum colorDifuso = material->Kd.GetColor(intersectInfo);

	Point3f lightPosition = light->getWorldPosition();

	Vector3f lightVector = Vector3f(lightPosition[0], lightPosition[1], lightPosition[2]);
	Vector3f intersectVector = Vector3f(intersectInfo.position[0], intersectInfo.position[1], intersectInfo.position[2]);
	Vector3f distanceVector = intersectVector - lightVector;
	float squareLightDistanceToCollision = distanceVector[0] * distanceVector[0] + distanceVector[1] * distanceVector[1] + distanceVector[2] * distanceVector[2];

	IntersectInfo infoShadow;
	Ray shadowRay = generateRay(intersectInfo.position, lightPosition);
	gmtl::Vec3f direction(lightPosition - intersectInfo.position);
	world->intersect(infoShadow, shadowRay, gmtl::length(direction));
	if (infoShadow.objectID == InvalidObjectID)
	{
		Vector3f directionLight = lightPosition - intersectInfo.position;

		Vector3f normalNormalized = intersectInfo.normal;
		normalize(normalNormalized);

		normalize(directionLight);

		float productoEscalar = gmtl::dot(normalNormalized, directionLight);

		float valorCoseno = productoEscalar > 0.0f ? productoEscalar : 0.0f;

		return Spectrum(colorDifuso[0] * (light->mIntensity / squareLightDistanceToCollision) * valorCoseno,
			colorDifuso[1] * (light->mIntensity / squareLightDistanceToCollision) * valorCoseno,
			colorDifuso[2] * (light->mIntensity / squareLightDistanceToCollision) * valorCoseno);
	}
	
	return Spectrum(0.0f, 0.0f, 0.0f);

}


Spectrum calculateSpecularComponent(World* world, PointLight* light, IntersectInfo& intersectInfo)
{
	Standard* material = (Standard *)intersectInfo.material;
	Spectrum colorEspecular = material->Ks.GetColor(intersectInfo);

	Vector3f directionNormalized = -intersectInfo.ray.getDir();
	normalize(directionNormalized);

	Point3f lightPosition = light->getWorldPosition();
	Vector3f lightVector = Vector3f(lightPosition[0], lightPosition[1], lightPosition[2]);
	Vector3f intersectVector = Vector3f(intersectInfo.position[0], intersectInfo.position[1], intersectInfo.position[2]);
	Vector3f distanceVector = intersectVector - lightVector;
	float squareLightDistanceToCollision = distanceVector[0] * distanceVector[0] + distanceVector[1] * distanceVector[1] + distanceVector[2] * distanceVector[2];

	IntersectInfo infoShadow;
	Ray shadowRay = generateRay(intersectInfo.position, lightPosition);
	gmtl::Vec3f direction(lightPosition - intersectInfo.position);
	world->intersect(infoShadow, shadowRay, gmtl::length(direction));
	if (infoShadow.objectID == InvalidObjectID)
	{
		Vector3f directionLight = lightPosition - intersectInfo.position;

		Vector3f normalNormalized = intersectInfo.normal;
		normalize(normalNormalized);

		normalize(directionLight);

		float escalarProduct = (gmtl::dot(normalNormalized, directionLight));
		Vector3f vectorInterm1 = escalarProduct * normalNormalized;
		Vector3f vectorInterm2 = 2.0f * vectorInterm1;
		Vector3f rVector = vectorInterm2 - directionLight;
		normalize(rVector);

		float productoEscalar = gmtl::dot(rVector, directionNormalized);

		float valorCoseno = productoEscalar > 0.0f ? pow(productoEscalar, material->Kshi) : 0.0f;
	

		return Spectrum(colorEspecular[0] * (light->mIntensity / squareLightDistanceToCollision) * valorCoseno,
			colorEspecular[1] * (light->mIntensity / squareLightDistanceToCollision) * valorCoseno,
			colorEspecular[2] * (light->mIntensity / squareLightDistanceToCollision) * valorCoseno);
	}

	return Spectrum(0.0f, 0.0f, 0.0f);

}

Spectrum traceRay(World* world, Ray& ray)
{
	IntersectInfo info;

	world->intersect(info, ray);

	if (info.objectID != InvalidObjectID)
	{
		//float value = 0.0f;

		float difusoRojo = 0.0f;
		float difusoVerde = 0.0f;
		float difusoAzul = 0.0f;


		float especularRojo = 0.0f;
		float especularVerde = 0.0f;
		float especularAzul = 0.0f;


		Standard* material = (Standard *)info.material;
		Spectrum colorDifuso = material->Kd.GetColor(info);
		Spectrum colorEspecular = material->Ks.GetColor(info);


		for (int i = 0; i < world->mLights.size(); ++i)
		{
			PointLight* light = (PointLight*)world->mLights[i];

			// Calcular Iluminacion Difusa
			Spectrum diffuseColor = calculateDiffuseComponent(world, light, info);
			difusoRojo += diffuseColor[0];
			difusoVerde += diffuseColor[1];
			difusoAzul += diffuseColor[2];

			// Calcular Iluminacion especular
			Spectrum especularColor = calculateSpecularComponent(world, light, info);
			especularRojo += especularColor[0];
			especularVerde += especularColor[1];
			especularAzul += especularColor[2];
		}

		// A�adir luz ambiente
		float ambienteRojo = material->Ka_color.GetColor(info)[0] * AMBIENT_INTENSITY;
		float ambienteVerde = material->Ka_color.GetColor(info)[1] * AMBIENT_INTENSITY;
		float ambienteAzul = material->Ka_color.GetColor(info)[2] * AMBIENT_INTENSITY;

		return Spectrum(difusoRojo + especularRojo + ambienteRojo, 
			difusoVerde + especularVerde + ambienteVerde,
			difusoAzul + especularAzul + ambienteAzul);

	}
	else
	{
		return Spectrum(0.0f, 0.0f, 0.0f);
	}
}

void render_image(World* world, unsigned int dimX, unsigned int dimY, float* image, float* alpha)
{
	Camera* camera = world->getCamera();


	for (int i = 0; i < dimY; ++i)
	{
		for (int j = 0; j < dimX; ++j)
		{

			//Calcular rayo desde c�mara a pixel
			Ray ray = camera->generateRay(j, i);

			Spectrum totalColor = traceRay(world, ray);

			image[(i * dimX * 3) + (j * 3)] = totalColor[0];
			image[(i * dimX * 3) + (j * 3) + 1] = totalColor[1];
			image[(i * dimX * 3) + (j * 3) + 2] = totalColor[2];
			
		}
	}


	/*for (int i = 0; i < dimX * dimY * 3; i += 3)
	{

	pixelX = i % dimX;
	pixelY = i / dimX;


	Ray ray = camera->generateRay(pixelX, pixelY);

	IntersectInfo info;

	world->intersect(info, ray);

	if (info.objectID != InvalidObjectID)
	{
	image[i] = FLT_MAX;
	image[i + 1] = FLT_MAX;
	image[i + 2] = FLT_MAX;
	}
	else
	{
	image[i] = 0.0f;
	image[i + 1] = 0.0f;
	image[i + 2] = 0.0f;
	//alpha[(i * (dimX * 3)) + j + 3] = 1;
	}

	}*/


	/*for (int i = 0; i < dimY; ++i)
	{
	for (int j = 0; j < dimX ; ++j)
	{
	Ray ray = camera->generateRay(j, i);

	IntersectInfo info;

	world->intersect(info, ray);

	if (info.objectID != InvalidObjectID)
	{
	image[(i * (dimX * 3)) + j] = FLT_MAX;
	//image[(i * (dimX * 3)) + j + 1] = FLT_MAX;
	//image[(i * (dimX * 3)) + j + 2] = FLT_MAX;
	//alpha[(i * (dimX * 3)) + j + 3] = 1;
	}
	else
	{
	image[(i * (dimX * 3)) + j] = 0.0f;
	//image[(i * (dimX * 3)) + j + 1] = 0.0f;
	//image[(i * (dimX * 3)) + j + 2] = 0.0f;
	//alpha[(i * (dimX * 3)) + j + 3] = 1;
	}
	}
	}*/
}

unsigned int g_intersectTriangleCalls;
extern "C"
{
	__declspec(dllexport) void renderSceneFromFile(float*& image, float*& alpha, World*& world, const char* filename)
	{
		google::InitGoogleLogging("rendering.dll");
		FLAGS_logtostderr = 1;

		g_intersectTriangleCalls = 0;

		// Create world from file
		world = ReadFromFile(filename);
		if (!world)
		{
			fprintf(stderr, "Error reading file %s. Press enter to exit", filename);
			getc(stdin);
			return;
		}
		INITREPORTER("report.ma", world);
		unsigned int dimX = world->getCamera()->getResolution()[0];
		unsigned int dimY = world->getCamera()->getResolution()[1];

		image = new float[dimX*dimY * 3];
		alpha = new float[dimX*dimY];


		// Compute pixel values
		clock_t tStart = clock();
		render_image(world, dimX, dimY, image, alpha);
		clock_t tEnd = clock();
		LOG(INFO) << "Time taken: " << (double)(tEnd - tStart) / CLOCKS_PER_SEC << "s";
		LOG(INFO) << "Triangles intersected: " << g_intersectTriangleCalls;

		google::ShutdownGoogleLogging();
	}

	__declspec(dllexport) void WriteImg(const char* name, float *pixels, float *alpha, int xRes,
		int yRes, int totalXRes, int totalYRes, int xOffset, int yOffset)
	{
		WriteImage(name, pixels, alpha, xRes, yRes, totalXRes, totalYRes, xOffset, yOffset);
	}
}

// dllmain.cpp : Defines the entry point for the DLL application.

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}