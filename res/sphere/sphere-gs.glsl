// This is an implementation of the method by Mara and McGuire for computing tight polyhedral bounds of a 3D sphere under perspective projection.
// It is based on their source code available at the link below, with only slight adaptations.
// 
// Michael Mara and Morgan McGuire. 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. 
// Journal of Computer Graphics Techniques, vol. 2, no. 2, pp. 70--83, 2013.
// https://research.nvidia.com/publication/2d-polyhedral-bounds-clipped-perspective-projected-3d-sphere

#version 450

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform float radiusScale;
uniform float clipRadiusScale;
uniform float nearPlaneZ = -0.125;

/** The number of sides in the bounding polygon. Must be even. */
#define N 4

/** The number of axes to caclulate bounds along. Two bounds per axis */
#define AXIS_NUM (N/2)
#define PI (3.1415926)

layout(points) in;
layout(triangle_strip, max_vertices = N) out;

out vec4 gFragmentPosition;
flat out vec4 gSpherePosition;
flat out float gSphereRadius;
flat out uint gSphereId;

/** 2D-line from point and direction */
struct line2D
{
    vec2 point;
    vec2 direction;
};

/** Simple line-line intersection in 2D. We don't handle 
    parallel lines, since they cannot arise in our implementation */
vec2 intersect(line2D L1, line2D L2)
{
    float denominator = (L1.direction.x * L2.direction.y) - (L1.direction.y * L2.direction.x);
    float leftTerm  =   (L1.point.x + L1.direction.x) * L1.point.y - (L1.point.y + L1.direction.y) * L1.point.x;
    float rightTerm =   (L2.point.x + L2.direction.x) * L2.point.y - (L2.point.y + L2.direction.y) * L2.point.x;
    vec2 numerator = leftTerm * L2.direction - rightTerm * L1.direction;
    return (numerator / denominator);
}

float square(float x)
{
    return x*x;
}

/** 
    Calculates the unprojected upper and lower bounds along axis determined by phi.
    perpendicularDirection is the direction in the xy plane perpendicular to the axis.
    The two lines running in perpendicularDirection through U and L determine two bounding lines
    in a screen-space bounding polygon for the sphere(center,radius) projected onto the screen.
    center, U, and L are all in camera-space. 

    Precondition: sphere not culled by near-plane 
*/
void getBoundsForPhi(in float phi, in vec3 center, in float radius, in float nearZ, out vec2 perpendicularDirection, out vec3 U, out vec3 L)
{
    bool trivialAccept = (center.z + radius) < nearZ; // Entirely in back of nearPlane (Trivial Accept)

    vec3 a = vec3(cos(phi), sin(phi), 0);
    perpendicularDirection.x = -a.y;
    perpendicularDirection.y = a.x;

    // given in coordinates (a,z), where a is in the direction of the vector a, and z is in the standard z direction
    vec2 projectedCenter = vec2(dot(a, center), center.z);  
    vec2 bounds_az[2];

    float projCenterSqLength = dot(projectedCenter, projectedCenter);
    float tSquared = dot(projectedCenter, projectedCenter) - square(radius);
    float costheta, sintheta;

    if(tSquared >  0)
	{
		// Camera is outside sphere
        // Distance to the tangent points of the sphere (points where a vector from the camera are tangent to the sphere) (calculated a-z space)
        float t = sqrt(tSquared);
        float invCLength = inversesqrt(projCenterSqLength);

        // Theta is the angle between the vector from the camera to the center of the sphere and the vectors from the camera to the tangent points
        costheta = t * invCLength;
        sintheta = radius * invCLength;
    }

    float sqrtPart;

    if(!trivialAccept)
		sqrtPart = sqrt(square(radius) - square(nearZ - projectedCenter.y));

    for( int i = 0; i < 2; ++i )
	{
        // Depending on the compiler and platform, it may be possible to optimize for 
        // performance by expanding out this caculation and using temporary variables 
        // for costheta^2 and costheta*sintheta
        if(tSquared >  0)
		{   
            // Matrices are column-major in GLSL
            mat2 rotateTheta = mat2(costheta,   sintheta, -sintheta,   costheta);
            bounds_az[i] = costheta * (rotateTheta * projectedCenter);
        } 

        if(!trivialAccept && (tSquared <= 0 || bounds_az[i].y > nearZ)) {
            bounds_az[i].x = projectedCenter.x + sqrtPart;
            bounds_az[i].y = nearZ; 
        }
        sintheta *= -1; // negate theta for B
        sqrtPart *= -1; // negate sqrtPart for B
    }

    U   = bounds_az[0].x * a;
    U.z = bounds_az[0].y;
    L   = bounds_az[1].x * a;
    L.z = bounds_az[1].y;
}

/** 
    Calculates the upper and lower bounds along axis determined by phi; 
    directly on the maxZ plane (which eliminates the need for a subsequent 
    projection). See http://www.terathon.com/code/scissor.html or 
    http://www.gamasutra.com/view/feature/2942/the_mechanics_of_robust_stencil_.php
    for a derivation. The Z coordinate for U and L is omitted, since it is always maxZ.
    This eliminates the need for explicit projection, which saves a division and two multiplies.

    In practical testing, using this method instead of our default implementation results in anywhere from a
    2%-30% slowdown, depending on the test harness parameters, on a GeForce TITAN with driver version 320.49.

    This version uses many less multiply-adds and multiplies than the default implementation 
    (and one less inverse square root), but 2-3 more divisions depending on whether the sphere intersects
    the near plane (even without requiring the final projection).

    We recommend testing and optimizing both methods on your target architecture if you need to eke out
    maximal performance for this function.
*/
void getBoundsForPhiLengyel(in float phi, in vec3 center, in float radius, in float maxZ, in float nearZ, out vec2 perpendicularDirection, out vec2 U, out vec2 L)
{
    bool trivialAccept = (center.z + radius) < nearZ; // Entirely in back of nearPlane (Trivial Accept)

    vec2 a = vec2(cos(phi), sin(phi));
    perpendicularDirection.x = -a.y;
    perpendicularDirection.y = a.x;

    // given in coordinates (a,z), where a is in the direction of the vector a, and z is in the standard z direction
    vec2 projectedCenter = vec2(dot(a, center.xy), center.z);  
    float bounds_a[2];
    float projCenterSqLength = dot(projectedCenter, projectedCenter);
    float tSquared = projCenterSqLength - square(radius);
    float t;
    float rC_a, normalCalculationSqrtPart, N_a_denominator;
    
	if(tSquared >  0)
	{ // Camera is outside sphere
        // Distance to the tangent points of the sphere (points where a vector from the camera are tangent to the sphere) (calculated a-z space)
        t                           = sqrt(tSquared);
        rC_a                        = radius * projectedCenter.x;
        normalCalculationSqrtPart   = projectedCenter.y * t;
        N_a_denominator             = 1 / projCenterSqLength;
    }
    
	float sqrtPart, invC_z;
    
	if(!trivialAccept)
	{
        sqrtPart = sqrt(square(radius) - square(nearZ - projectedCenter.y));
        invC_z = 1.0 / projectedCenter.y;
    }

    for(int i = 0; i < 2; ++i )
	{
        float N_a;

        if(tSquared >  0)
		{   
            // Calculate the 'a' coordinate. This is Q_x in the terathon article (maxZ is the "e" value).
			N_a = N_a_denominator * (rC_a + normalCalculationSqrtPart);
            bounds_a[i] = -maxZ * (radius - N_a * projectedCenter.x) / ( N_a * projectedCenter.y );
        } 

        if(!trivialAccept)
		{ 
            bool replace = tSquared <= 0;
            
			if (!replace)
			{ // Do the extra work to find the Z-coordinate of the tangent point
                float N_z = (radius - N_a * projectedCenter.x) * invC_z;
                float z = projectedCenter[1] - radius * N_z;
                replace = z > nearZ;
            }
            
			if ( replace )
			{ 
                bounds_a[i] = projectedCenter.x + sqrtPart;
            }
        }
        normalCalculationSqrtPart *= -1;
        sqrtPart *= -1; // negate sqrtPart for B
    }
    U   = bounds_a[0] * a;
    L   = bounds_a[1] * a;
}


struct Element
{
	vec3 color;
	float radius;
};

layout(std140, binding = 0) uniform elementBlock
{
	Element elements[32];
};

void main()
{
	uint sphereId = floatBitsToUint(gl_in[0].gl_Position.w);
	uint elementId = bitfieldExtract(sphereId,0,8);
	float sphereRadius = elements[elementId].radius*radiusScale;
	float sphereClipRadius = elements[elementId].radius*clipRadiusScale;
	
	gSphereId = sphereId;
	gSpherePosition = gl_in[0].gl_Position;
	gSphereRadius = sphereRadius;

	vec4 c = modelViewMatrix * vec4(gl_in[0].gl_Position.xyz,1.0);
	
	vec4 size = modelViewMatrix * vec4(sphereRadius,0.0,0.0,0.0);
	float radius = length(size);

	vec4 clipSize = modelViewMatrix * vec4( sphereClipRadius, 0.0,0.0,0.0);
	float clipRadius = length(clipSize);

	if (c.z + clipRadius >= nearPlaneZ)
		return;
	
    // We'll duplicate the first line into the last spot to avoid modular arithmetic while looping
    line2D  boundingLines[N + 1];
    float invAxisNum = 1.0 / AXIS_NUM;

    // The plane we draw the bounds on
    float maxZ = min(nearPlaneZ, c.z);

// If we use our default method, we need to project onto the maxZ plane, the lengyel 
// method just solves for the x-y coordinates directly on the maxZ plane.
#if USE_LENGYEL_METHOD == 0
    vec3    axesBounds[N];
    for(int i = 0; i < AXIS_NUM; ++i)
	{
        float phi = (i * PI) * invAxisNum;
        getBoundsForPhi(phi, c.xyz, radius, nearPlaneZ, boundingLines[i].direction, axesBounds[i], axesBounds[i + AXIS_NUM]);
        boundingLines[i + AXIS_NUM].direction = boundingLines[i].direction;
    }
    for(int i = 0; i < N; ++i)
	{
        boundingLines[i].point = axesBounds[i].xy * (maxZ / axesBounds[i].z);
    }
#else
    for(int i = 0; i < AXIS_NUM; ++i)
	{
        float phi = (i * PI) * invAxisNum;
        getBoundsForPhiLengyel(phi, c.xyz, radius, maxZ, nearPlaneZ, boundingLines[i].direction, boundingLines[i].point, boundingLines[i + AXIS_NUM].point);
        boundingLines[i + AXIS_NUM].direction = boundingLines[i].direction;
    }
#endif

    boundingLines[N] = boundingLines[0]; // Avoid modular arithmetic
    
    // The funky ordering is because we are emitting a triangle strip
    for(int i = 0; i < AXIS_NUM; ++i)
	{
        int j = N - i - 1;
        vec4 pos = vec4(intersect(boundingLines[j], boundingLines[j+1]), maxZ, 1.0f);
        
		gFragmentPosition = projectionMatrix * pos;
		gl_Position = gFragmentPosition;
        EmitVertex();

        pos = vec4(intersect(boundingLines[i], boundingLines[i+1]), maxZ, 1.0f);
		gFragmentPosition = projectionMatrix * pos;
		gl_Position = gFragmentPosition;
        EmitVertex();
    }
    

    EndPrimitive();
}