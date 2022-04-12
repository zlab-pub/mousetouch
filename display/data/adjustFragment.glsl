#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec3 Position;

uniform sampler2D texture0;
uniform sampler2D rawScreen;
uniform int letThrough;
// uniform int baseColor;
// uniform float pct;

void main()
{
    // Position ranges [-1.0, 1.0].
    vec4 rawColor = texture(rawScreen, (Position.xy + vec2(1.0, 1.0)) / vec2(2,2));
    // float c = baseColor;
    // rawColor = vec4(c/255.0, c/255.0, c/255.0, 1.0);
    rawColor = rawColor * 100.0f / 255.0f + vec4(80.0f/255.0f, 80.0f/255.0f,80.0f/255.0f,0);
    if (letThrough == 1) {
        FragColor = rawColor;
        FragColor.w = 1.0f;
    } else {
        float temp = texture(texture0, TexCoord).x;
        float temp2 = texture(texture0, TexCoord).y;
        float temp3 = texture(texture0, TexCoord).z;
        float pct = 30.0 / 255.0 / ((rawColor.x + rawColor.y + rawColor.z) / 3.0);
        FragColor = vec4(rawColor.x *(1- pct*(1.0f/0.3f)*( temp * 2 - 1)), 
           rawColor.y *(1- pct*(1.0f/0.3f)*( temp * 2 - 1)), 
           rawColor.z *(1- pct*(1.0f/0.3f)*( temp * 2 - 1)), 1.0);
        // float pct = 15.0 / 255.0 / ((temp +temp2 + temp3) / 3.0);
        // FragColor = vec4(temp *(1- pct*(1.0f/0.3f)*( rawColor.x * 2 - 1)), 
        //    temp2 *(1- pct*(1.0f/0.3f)*( rawColor.y * 2 - 1)), 
        //    temp3 *(1- pct*(1.0f/0.3f)*( rawColor.z * 2 - 1)), 1.0);
            // FragColor = vec4(temp * rawColor.x * 2,temp * rawColor.y * 2,temp * rawColor.z * 2, 1.0);
    }
    //vec4(texture(texture0, TexCoord).xyz, texture(texture0, TexCoord).w) * (1 - letThrough);
    //caution: FragColor.w should always be newColor.w so the gl_blend will be correct
}
