#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Position;
uniform sampler2D texture0;
uniform int nbFrame;

void main()
{
  const int speed = 1;
  const int sep = 5;
  const vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
  int xx = int(Position.x * 960 + 960);
  int yy = int(Position.y * 540 + 540);
  if ((xx % sep == (nbFrame * speed) % sep)
      // && (yy % sep == (nbFrame * speed) % sep)  
    ) {
    FragColor = color;
  } else {
    FragColor = vec4(0.0,0.0,0.0,1.0);
  }

}