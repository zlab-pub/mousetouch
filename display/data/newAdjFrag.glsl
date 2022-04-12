#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec3 Position;

uniform sampler2D texture0;
uniform sampler2D rawScreen;
uniform int letThrough;
uniform int nbFrames;
// uniform int baseColor;
// uniform float pct;

void main()
{
    vec4 rawColor = texture(rawScreen, (Position.xy + vec2(1.0, 1.0)) / vec2(2,2));
    rawColor = rawColor * 100.0f / 255.0f + vec4(80.0f/255.0f, 80.0f/255.0f,80.0f/255.0f,0);
    if (letThrough == 1) {
        FragColor = rawColor;
        FragColor.w = 1.0f;
    } else {
        int x, y;
        y = int(TexCoord.x * 32);
        x = int(TexCoord.y * 32);
        int coding[10];
        coding[0] = 0;
        int crc[10];
        crc[0] = 0;
        for (int i = 1; i < 8; i++) {
          coding[i] = ((x % i) ^ ((y % i));
          crc[i] = coding[i] ^ crc[i - 1];
        }
        for (int i = 8; i < 10; i++) {
          coding[i] = crc [i-10];
        }
        FragColor = rawColor - (coding[nbFrames % 62]) * 1.0f;  
        // FragColor = rawColor - 80.0f;
        FragColor.w = 1.0f;
    }
    //vec4(texture(texture0, TexCoord).xyz, texture(texture0, TexCoord).w) * (1 - letThrough);
    //caution: FragColor.w should always be newColor.w so the gl_blend will be correct
}
