#version 300 es
precision highp float;

layout (location = 0) out mediump vec4 o_Color;
uniform mediump sampler2D u_MainTex;
in mediump vec2 v_TexCoord;
layout(std140) uniform ub_Scene
{
	highp vec4 iu_resolution;		// x - width; y - height; z - fragmentWidth; w - fragmentHeight
	highp vec4 iu_timeSeconds;		// x - timeElapsed; y - deltaTime
};

// https://www.shadertoy.com/view/4tlBz8
// org: https://www.shadertoy.com/view/lsBfDz

#define NUM_STEPS 200
#define NUM_NOISE_OCTAVES 4

#define HEIGHT_OFFSET 1.25

float RandomNumber (in vec3 position)
{
    // NOTE: the ceil here is important interestingly. it makes the clouds look round and puffy instead of whispy in a glitchy way
    vec2 uv = (position.yz+ceil(position.x))/float(NUM_STEPS);
	
	return texture(u_MainTex, uv).y;
}

void main()
{
    // x,y,z is the direction the ray for this pixel travels in.
    // x is into the screen
    // y is the screen's x axis. the left side is -0.8 and the right side value depends on the aspect ratio seems to be be roughly +0.8 for me.
    // z is the screen's y axis (also the up axis). it ranges from -0.8 at the bottom of the screen to 0.2 at the top of the screen
    // NOTE: in tiny clouds, this is a vec4 where the y field is unused. I've made it a vec3 and removed the y field.
    vec3 direction = vec3(0.8, v_TexCoord * iu_resolution.xy / iu_resolution.y-0.8);

    // this is a sky blue color
    // NOTE: tiny clouds gets the 0.8 from direction.x
    vec3 skyColor = vec3(0.6, 0.7, 0.8);
    
    // initialize the pixel color to a gradient of sky blue (at top) to white (at bottom)
    vec3 pixelColor = skyColor - direction.z;
    
    // Tiny clouds adds a per pixel value to rayStep that is in [-1,1].
    // I think that gives it some higher frequency noise to make the clouds look a little more detailed.  
    // NOTE: this ray marches back to front so that alpha blending math is easier
    for (int rayStep = 0; rayStep < NUM_STEPS; ++rayStep)
    {
        // NOTE: tiny clouds has position as a vec4 but never uses the y component. I removed it here.
        // position.z is up
        vec3 position = 0.05 * float(NUM_STEPS - rayStep) * direction;
        
        // move the camera on the x and z axis over time
        position.xy+=iu_timeSeconds.x;
        
        // tiny clouds initializes this to 2.0, but whenever using it divides it by 4. So, just dividing 2 by 4 here.
        float noiseScale=0.5;
        
        // Note: position.z is the height. adding this moves the camera up. tiny clouds uses 1.0
        float signedCloudDistance = position.z + HEIGHT_OFFSET;
        
        // Note: each sampling doubles the position but halves the value read there. it's multiple octaves of noise i think?        
        for (int octaveIndex = 0; octaveIndex < NUM_NOISE_OCTAVES; ++octaveIndex)
        {
            position *= 2.0;
            noiseScale *= 2.0;
            signedCloudDistance -= RandomNumber(position) / noiseScale;
        }
        
        // Note: signed cloud distance is also cloud density if negative.
        // the below is equivelant to a lerp between the current pixel color and the color that the cloud is at that location.
        // the lerp amount is controlled by the density time 0.4.
        // The color is reversed so it "looks like" the sky color.
        // Lerping is equivelant to standard alpha blending, so we are just doing alpha compositing.
        if (signedCloudDistance < 0.0)
            pixelColor = pixelColor + (pixelColor - 1.0 - signedCloudDistance * skyColor.zyx)*signedCloudDistance*0.4;
	}
    
    o_Color = vec4(pixelColor, 1.0);
}