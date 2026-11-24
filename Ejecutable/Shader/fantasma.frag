#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;   // la misma textura del esqueleto

uniform vec3  viewPos;
uniform vec3  colorFantasma;          // azul-blanco
uniform float alphaFantasma;          // 0 = invisible, 1 = completamente visible
uniform float tiempo;

uniform vec3  colorNiebla;
uniform float densidadNiebla;

void main()
{
    vec4 tex = texture(texture_diffuse1, TexCoords);
    if (tex.a < 0.1)
        discard;

    // Textura teñida de un solo color
    float lum = dot(tex.rgb, vec3(0.299, 0.587, 0.114));

    // Fresnel: bordes más brillantes
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    float fres = pow(1.0 - abs(dot(N, V)), 2.0);

    vec3 col = colorFantasma * (0.25 + 0.6 * lum) + colorFantasma * fres * 1.2;

    // Ondas de luz que suben por el cuerpo
    float onda = 0.5 + 0.5 * sin(FragPos.y * 6.0 - tiempo * 3.0);

    // Parpadeo suave
    float parpadeo = 0.85 + 0.15 * sin(tiempo * 7.0);

    float alpha = alphaFantasma * (0.35 + 0.45 * fres) * (0.8 + 0.2 * onda) * parpadeo;

    // Niebla
    float d = length(viewPos - FragPos);
    float f = clamp(exp(-pow(d * densidadNiebla, 2.0)), 0.0, 1.0);
    alpha *= f;

    FragColor = vec4(col, alpha);
}