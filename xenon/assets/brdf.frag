#version 460 core

// NOTE: Source based on the work of JoeyDeVries (Github)

//---------------------------------------------------------------
// [SECTION] Input data & output variables
//---------------------------------------------------------------

out vec4 fragColor;

in vec2 textureCoord;


//---------------------------------------------------------------
// [SECTION] Constants
//---------------------------------------------------------------

#define PI 3.14159265359
#define EPSILON 0.0000001


//---------------------------------------------------------------
// [SECTION] Efficient VanDerCorpus calculation
// http://holger.dammertz.org/stuff/notes_hammersleyOnHemisphere.html
//---------------------------------------------------------------

float radicalInverse_VdC(uint bits)  {
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}


//---------------------------------------------------------------
// [SECTION] Hammersley
//---------------------------------------------------------------

vec2 hammersley(uint i, uint N) {
	return vec2(float(i)/float(N), radicalInverse_VdC(i));
}


//---------------------------------------------------------------
// [SECTION] Importance Sample GGX
//---------------------------------------------------------------

vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
	float a = roughness*roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// From spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// From tangent-space H vector to world-space sample vector
	vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

//---------------------------------------------------------------
// [SECTION] Geometry Schlick-GGX
//---------------------------------------------------------------

float geometrySchlickGGX(float NdotV, float roughness) {
	// Mote that we use a different k for IBL
	float a = roughness;
	float k = (a * a) / 2.0;

	float nom   = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

//---------------------------------------------------------------
// [SECTION] Smith's method using Schlick-GGX geometry function
//---------------------------------------------------------------

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = geometrySchlickGGX(NdotV, roughness);
	float ggx1 = geometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}


//---------------------------------------------------------------
// [SECTION] Integrate BRDF
//---------------------------------------------------------------

vec2 IntegrateBRDF(float NdotV, float roughness) {
	vec3 V;
	V.x = sqrt(1.0 - NdotV*NdotV);
	V.y = 0.0;
	V.z = NdotV;

	float A = 0.0;
	float B = 0.0; 

	vec3 N = vec3(0.0, 0.0, 1.0);
	
	const uint SAMPLE_COUNT = 1024u;

	for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
		// Generates a sample vector that's biased towards the
		//  preferred alignment direction (importance sampling).
		vec2 Xi = hammersley(i, SAMPLE_COUNT);
		vec3 H = importanceSampleGGX(Xi, N, roughness);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(L.z, 0.0);
		float NdotH = max(H.z, 0.0);
		float VdotH = max(dot(V, H), 0.0);

		if(NdotL > 0.0) {
			float G = geometrySmith(N, V, L, roughness);
			float G_Vis = (G * VdotH) / (NdotH * NdotV);
			float Fc = pow(1.0 - VdotH, 5.0);

			A += (1.0 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}
	A /= float(SAMPLE_COUNT);
	B /= float(SAMPLE_COUNT);
	return vec2(A, B);
}


//---------------------------------------------------------------
// [SECTION] Main program
//---------------------------------------------------------------

void main() {
	vec2 integratedBRDF = IntegrateBRDF(textureCoord.x, textureCoord.y);
	fragColor = vec4(integratedBRDF, 0.0, 1.0);
}
