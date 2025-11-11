#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    // Sample the texture
    FragColor = texture(screenTexture, TexCoords);
    // Discard fragments that are fully transparent
    if(FragColor.a < 0.1)
        discard;
}