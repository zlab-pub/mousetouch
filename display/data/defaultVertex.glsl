#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 Position;

uniform float xTrans, yTrans;
uniform int centrosymmetric = 0;

void main()
{

    Position = vec3(aPos.x + xTrans, aPos.y + yTrans, aPos.z);
    TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    if (centrosymmetric == 1) {
        Position.x = -Position.x;
        Position.y = -Position.y; 

        //vertex do the centrosymmetric mapping but texture do not.
        TexCoord.x = 1.0 - TexCoord.x;
        TexCoord.y = 1.0 - TexCoord.y;
    }
    gl_Position = vec4(Position.x, Position.y, Position.z, 1.0);
    
}