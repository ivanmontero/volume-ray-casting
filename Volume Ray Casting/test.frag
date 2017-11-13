#version 330 core

smooth in vec2 uv;

out vec4 color;

// Camera
uniform float near;
uniform float far;
uniform int width;
uniform int height;
uniform float aspectRatio;
uniform vec3 camUp;
uniform vec3 camRight;
uniform vec3 camFront;
uniform vec3 eye;
uniform float focalLength;

// Raymarch
uniform float epsilon;	// Distance treshhold 
uniform int maxSteps;

// Rotates a point around the X axis
vec3 rotateX(vec3 v, float t) {
    float c = cos(t);
    float s = sin(t);
    return mat3(
        vec3(1, 0, 0),
        vec3(0, c, -s),
        vec3(0, s, c)
    ) * v;
}

// Rotates a point around the Y axis
vec3 rotateY(vec3 v, float t) {
    float c = cos(t);
    float s = sin(t);
    return mat3(
        vec3(c, 0, s),
        vec3(0, 1, 0),
        vec3(-s, 0, c)
    ) * v;
}

// Rotates a point around the Z axis
vec3 rotateZ(vec3 v, float t) {
    float c = cos(t);
    float s = sin(t);
    return mat3(
        vec3(c, -s, 0),
        vec3(s, c, 0),
        vec3(0, 0, 1)
    ) * v;
}

// Signed distance box
float sdBox(vec3 p, vec3 b){
	vec3 d = abs(p) - b;
	return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float sdSphere(vec3 p, float r) {
	return length(p) - r;
}

// Helper method for translations
vec3 translate(vec3 v, vec3 t) {
	return v - t;
}
// To apply transformations, the inverse transformation must
// be applied.
// Translation: Negate (inverse) components + -> -
// Rotation: Opposite transform
// Scale (uniform): Inverse operation (* -> /)
//		- Scale affects the the distance function, therefore
//		  the opposite of the applied operation must be applied
//		  to the scaling factor (or SDF)  to preserve distance

// Distance field for the scene
float distScene(vec3 p) {
	// Sphere
	 //vec3 is a translation
	//return length(translate(p, vec3(0, -.5, 0))) - .5;
	// return length(p) - .5;

	// repeated spheres
	// float c = 2;		// distance?
	// p.x = mod(p.x, 1 * c) - .5 * c;
	// p.y = mod(p.y, 1 * c) - .5 * c;
	// p.z = mod(p.z, 1 * c) - .5 * c;
	// return length(p) - .5f;
	
	// Psychedelic
	// V1
	// p.x = mod(-abs(p.x), 1) - 1;
	// p.y = mod(-abs(p.y), 1) - 1;
	// p.z = mod(-abs(p.z), 1) - 1;
	// return length(p) - .5;
	// V2
	p.x = mod(p.x, 1) - 1;
	p.y = mod(p.y, 1) - 1;
	p.z = mod(p.z, 1) - 1;
	return length(p) - .5;

	// Box (twist)
	// return sdBox(rotateY(p, p.y), vec3(1, 3, 2));

}

// Finds the closest intersecting object along origin ro, and direction rd
// ro: ray origin
// rd: ray direction
// i: step count
// t: distance traveled
void raymarch(vec3 ro, vec3 rd, out int i, out float t) {
	t = 0.0f;
	for(i = 0; i < maxSteps; i++) {
		// Signed volume function for the scene here
		float dist = distScene(ro + rd * t);

		// Makes epsilon proportional to t to drop accuracy the further into the scene it gets.
		// Also drops the ray if greater than far.
		if(dist < epsilon * t * 2.0f || t > far)
			break;

		//if(dist < epsilon)
		//	break;

		t += dist;
	}
}

vec4 computeColor(vec3 ro, vec3 rd) {
	float t;
	int i;
	raymarch(ro, rd, i, t);
	if(t > far)
		return vec4(0.0f);
	float c = float(i)/maxSteps;
	return vec4(c, c, c, 1.0f);

}

void main() {
	// Ray origin
	vec3 ro = eye;	
	// Ray direction
	vec3 rd = normalize(camFront * focalLength + camRight * uv.x * aspectRatio + camUp * uv.y);

	color = computeColor(ro, rd);
}