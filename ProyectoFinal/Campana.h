#pragma once

// =============================================================
//  Campana.h  -  Campana del atrio que dobla por los difuntos
//
//  En la noche de muertos las campanas de la iglesia "doblan"
//  para llamar a las ánimas. Esta clase modela:
//
//  1) La campana como péndulo físico amortiguado:
//        theta'' = -(g / L) * sin(theta) - c * theta' + u(t)
//     u(t) es el empuje del campanero. Se aplica solo mientras
//     la energía de la campana es menor que la energía objetivo
//     (control por energía, igual que cuando se impulsa un columpio):
//        E = 1/2 * theta'^2 + (g / L) * (1 - cos(theta))
//        u = +k * signo(theta')   si  E < E_objetivo
//
//  2) El badajo como un segundo péndulo, más corto, que cuelga
//     del mismo eje:
//        phi'' = -(g / Lb) * sin(phi) - cb * phi'
//     Como sus periodos son distintos (T = 2*pi*sqrt(L/g)), el
//     badajo se desfasa y golpea la pared interior. El golpe se
//     resuelve con un coeficiente de restitución e:
//        (phi' - theta')_despues = -e * (phi' - theta')_antes
//
//  3) El sonido de cada golpe se sintetiza con los parciales
//     típicos de una campana (hum, prime, tierce, quint, nominal),
//     cada uno con decaimiento exponencial, y su volumen depende
//     de la fuerza del golpe y de la distancia al jugador.
//
//  La geometría de la campana y del badajo se genera por código
//  como superficie de revolución de un perfil 2D.
//  La integración usa Euler semi-implícito con pasos fijos de
//  1/240 s para que sea estable aunque cambien los FPS.
// =============================================================

// Librerías
#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Mesh.h"

class Campana
{
public:

	// ---------------- Parámetros físicos ----------------
	float gravedad = 9.81f;
	float largoCampana = 0.90f;          // longitud equivalente del péndulo físico (m)
	float largoBadajo = 0.72f;           // más corto: oscila más rápido y se desfasa
	float amortCampana = 0.18f;          // c  (1/s)
	float amortBadajo = 0.35f;           // cb (1/s)
	float empuje = 1.6f;                 // k  (rad/s^2) fuerza del campanero
	float amplitudObjetivo = glm::radians(62.0f);
	float restitucion = 0.35f;           // e  del choque badajo-campana
	float anguloContacto = 0.34f;        // ángulo relativo en el que el badajo toca la pared (rad)

	// ---------------- Colores ----------------
	glm::vec3 colorBronce = glm::vec3(0.55f, 0.38f, 0.16f);
	glm::vec3 colorBadajo = glm::vec3(0.18f, 0.16f, 0.14f);
	glm::vec3 colorMadera = glm::vec3(0.30f, 0.18f, 0.09f);
	glm::vec3 colorPiedra = glm::vec3(0.35f, 0.33f, 0.30f);

	Campana() {}

	~Campana()
	{
		delete mallaCampana;
		delete mallaBadajo;
	}

	// Se llama después de crear el contexto de OpenGL
	void inicializar(glm::vec3 posicionBase)
	{
		base = posicionBase;
		pivote = base + glm::vec3(0.0f, ALTURA_EJE, 0.0f);

		crearMallaCampana();
		crearMallaBadajo();
		crearSonido();
	}

	// Tecla C: empezar o dejar de doblar
	void alternarDoblar()
	{
		doblando = !doblando;

		// Si está en reposo se le da un primer jalón para que arranque
		if (doblando && std::fabs(theta) < 0.01f && std::fabs(omega) < 0.01f)
			omega = 0.35f;
	}

	bool estaDoblando() const { return doblando; }

	// Cajas de colisión de los postes: (minX, maxX, minZ, maxZ)
	std::vector<glm::vec4> cajasColision() const
	{
		std::vector<glm::vec4> cajas;
		for (int lado = -1; lado <= 1; lado += 2)
		{
			float z = base.z + lado * SEPARACION_POSTES;
			cajas.push_back(glm::vec4(base.x - 0.35f, base.x + 0.35f, z - 0.35f, z + 0.35f));
		}
		return cajas;
	}

	// Avanza la simulación
	void actualizar(float dt, glm::vec3 posJugador)
	{
		if (dt > 0.1f) dt = 0.1f;          // evita saltos grandes si la ventana se congela
		tiempoDesdeGolpe += dt;
		acumulado += dt;

		while (acumulado >= PASO)
		{
			paso(PASO, posJugador);
			acumulado -= PASO;
		}
	}

	// Dibuja campanario, campana y badajo con el shader de iluminación
	void dibujar(Shader& shader, GLint modelLoc, GLuint cuboVAO)
	{
		GLuint P = shader.Program;
		GLint locUsarColor = glGetUniformLocation(P, "usarColor");
		GLint locColor = glGetUniformLocation(P, "colorPlano");

		glUniform1i(glGetUniformLocation(P, "transparency"), 0);
		glUniform1i(locUsarColor, 1);

		// ---- Campanario de madera (cubo unitario escalado) ----
		glUniform3fv(locColor, 1, glm::value_ptr(colorMadera));
		glBindVertexArray(cuboVAO);
		for (int lado = -1; lado <= 1; lado += 2)
		{
			float z = base.z + lado * SEPARACION_POSTES;

			// Poste
			dibujarCubo(modelLoc, glm::vec3(base.x, ALTURA_VIGA * 0.5f, z), glm::vec3(0.22f, ALTURA_VIGA, 0.22f));

			// Tornapuntas (refuerzos diagonales)
			glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(base.x, ALTURA_VIGA - 0.32f, z - lado * 0.32f));
			m = glm::rotate(m, glm::radians(-lado * 45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			m = glm::scale(m, glm::vec3(0.12f, 0.9f, 0.12f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		// Viga superior
		dibujarCubo(modelLoc, glm::vec3(base.x, ALTURA_VIGA + 0.12f, base.z),
			glm::vec3(0.26f, 0.24f, 2.0f * SEPARACION_POSTES + 0.5f));

		// Bases de piedra de los postes
		glUniform3fv(locColor, 1, glm::value_ptr(colorPiedra));
		for (int lado = -1; lado <= 1; lado += 2)
		{
			float z = base.z + lado * SEPARACION_POSTES;
			dibujarCubo(modelLoc, glm::vec3(base.x, 0.15f, z), glm::vec3(0.5f, 0.3f, 0.5f));
		}

		// ---- Yugo: gira junto con la campana ----
		glm::mat4 giroCampana = glm::translate(glm::mat4(1.0f), pivote);
		giroCampana = glm::rotate(giroCampana, theta, glm::vec3(0.0f, 0.0f, 1.0f));

		glUniform3fv(locColor, 1, glm::value_ptr(colorMadera));
		glm::mat4 m = glm::translate(giroCampana, glm::vec3(0.0f, -0.02f, 0.0f));
		m = glm::scale(m, glm::vec3(0.30f, 0.20f, 2.0f * SEPARACION_POSTES - 0.25f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		// ---- Campana ----
		glUniform3fv(locColor, 1, glm::value_ptr(colorBronce));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(giroCampana));
		mallaCampana->Draw(shader);

		// ---- Badajo: gira con su propio ángulo phi ----
		glm::mat4 giroBadajo = glm::translate(glm::mat4(1.0f), pivote);
		giroBadajo = glm::rotate(giroBadajo, phi, glm::vec3(0.0f, 0.0f, 1.0f));
		glUniform3fv(locColor, 1, glm::value_ptr(colorBadajo));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(giroBadajo));
		mallaBadajo->Draw(shader);

		glUniform1i(locUsarColor, 0);
	}

private:

	// ---------------- Medidas del campanario ----------------
	const float ALTURA_VIGA = 4.2f;          // altura de la viga superior (m)
	const float ALTURA_EJE = 4.0f;           // altura del eje de giro (m)
	const float SEPARACION_POSTES = 1.2f;    // mitad de la distancia entre postes (m)
	const float PASO = 1.0f / 240.0f;        // paso fijo de integración (s)

	// ---------------- Estado ----------------
	glm::vec3 base = glm::vec3(0.0f);
	glm::vec3 pivote = glm::vec3(0.0f);
	float theta = 0.0f, omega = 0.0f;        // campana: ángulo y velocidad angular
	float phi = 0.0f, omegaB = 0.0f;         // badajo
	bool  doblando = false;
	float acumulado = 0.0f;
	float tiempoDesdeGolpe = 10.0f;

	Mesh* mallaCampana = nullptr;
	Mesh* mallaBadajo = nullptr;

	// ---------------- Sonido ----------------
	static const int FRECUENCIA_MUESTREO = 22050;
	static const int MUESTRAS = 22050 * 3;           // 3 s de sonido
	std::vector<float> ondaBase;                     // sonido normalizado (-1..1)
	std::vector<uint8_t> wav[2];                     // dos búferes: uno suena mientras se llena el otro
	int bufferActual = 0;

	// Un paso de la simulación
	void paso(float h, glm::vec3 posJugador)
	{
		// --- Campana: control por energía + péndulo amortiguado ---
		float w2 = gravedad / largoCampana;
		float energia = 0.5f * omega * omega + w2 * (1.0f - std::cos(theta));
		float energiaObjetivo = w2 * (1.0f - std::cos(amplitudObjetivo));

		float u = 0.0f;
		if (doblando && energia < energiaObjetivo)
			u = empuje * (omega >= 0.0f ? 1.0f : -1.0f);

		float alfa = -w2 * std::sin(theta) - amortCampana * omega + u;
		omega += alfa * h;                 // Euler semi-implícito: primero velocidad,
		theta += omega * h;                // luego posición con la velocidad nueva

		// --- Badajo: péndulo libre más corto ---
		float w2b = gravedad / largoBadajo;
		float alfaB = -w2b * std::sin(phi) - amortBadajo * omegaB;
		omegaB += alfaB * h;
		phi += omegaB * h;

		// --- Choque contra la pared interior ---
		float relativo = phi - theta;
		if (std::fabs(relativo) > anguloContacto)
		{
			float signo = (relativo > 0.0f) ? 1.0f : -1.0f;
			float velRel = omegaB - omega;

			// Solo si el badajo se sigue acercando a la pared
			if (velRel * signo > 0.0f)
			{
				omegaB = omega - restitucion * velRel;
				if (tiempoDesdeGolpe > 0.25f)
				{
					tocar(std::fabs(velRel), posJugador);
					tiempoDesdeGolpe = 0.0f;
				}
			}
			phi = theta + signo * anguloContacto;
		}
	}

	// ---------------- Geometría ----------------

	// Dibuja el cubo unitario escalado y trasladado
	void dibujarCubo(GLint modelLoc, glm::vec3 centro, glm::vec3 escala)
	{
		glm::mat4 m = glm::translate(glm::mat4(1.0f), centro);
		m = glm::scale(m, escala);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	// Gira un perfil 2D (r, y) alrededor del eje Y.
	// Las normales del perfil se calculan con la perpendicular (-dy, dr) de
	// cada segmento y se promedian en los vértices donde el perfil es suave.
	static Mesh* superficieRevolucion(const std::vector<glm::vec2>& perfil, int rebanadas)
	{
		const float COS_LIMITE = std::cos(glm::radians(40.0f));
		const float DOS_PI = 6.28318530718f;
		size_t n = perfil.size();

		// Normal 2D de cada segmento
		std::vector<glm::vec2> normalSeg(n - 1);
		for (size_t i = 0; i + 1 < n; i++)
		{
			glm::vec2 d = perfil[i + 1] - perfil[i];
			glm::vec2 nn(-d.y, d.x);
			float l = glm::length(nn);
			normalSeg[i] = (l > 1e-6f) ? nn / l : glm::vec2(0.0f, 1.0f);
		}

		// Normales al inicio y al final de cada segmento (suavizadas si el ángulo es pequeño)
		std::vector<glm::vec2> nIni(n - 1), nFin(n - 1);
		for (size_t i = 0; i + 1 < n; i++)
		{
			nIni[i] = normalSeg[i];
			nFin[i] = normalSeg[i];
			if (i > 0 && glm::dot(normalSeg[i - 1], normalSeg[i]) > COS_LIMITE)
				nIni[i] = glm::normalize(normalSeg[i - 1] + normalSeg[i]);
			if (i + 2 < n && glm::dot(normalSeg[i], normalSeg[i + 1]) > COS_LIMITE)
				nFin[i] = glm::normalize(normalSeg[i] + normalSeg[i + 1]);
		}

		std::vector<Vertex> vertices;
		std::vector<GLuint> indices;

		for (size_t i = 0; i + 1 < n; i++)
		{
			GLuint inicio = (GLuint)vertices.size();
			for (int s = 0; s <= rebanadas; s++)
			{
				float a = DOS_PI * s / rebanadas;
				float c = std::cos(a), sn = std::sin(a);

				for (int extremo = 0; extremo < 2; extremo++)
				{
					glm::vec2 p = perfil[i + extremo];
					glm::vec2 nr = extremo ? nFin[i] : nIni[i];
					Vertex v;
					v.Position = glm::vec3(p.x * c, p.y, p.x * sn);
					v.Normal = glm::normalize(glm::vec3(nr.x * c, nr.y, nr.x * sn));
					v.TexCoords = glm::vec2((float)s / rebanadas, (float)(i + extremo) / (n - 1));
					vertices.push_back(v);
				}
			}
			for (int s = 0; s < rebanadas; s++)
			{
				GLuint a0 = inicio + 2 * s, a1 = a0 + 1, b0 = a0 + 2, b1 = a0 + 3;
				indices.push_back(a0); indices.push_back(a1); indices.push_back(b1);
				indices.push_back(a0); indices.push_back(b1); indices.push_back(b0);
			}
		}

		return new Mesh(vertices, indices, std::vector<Texture>());
	}

	// Perfil de una campana tradicional: corona, hombro, cintura,
	// "anillo de sonido" grueso y labio. Se recorre el exterior hacia
	// abajo y el interior hacia arriba para obtener una pared con grosor.
	void crearMallaCampana()
	{
		// (h = profundidad desde la corona, r = radio exterior)
		const float h[] = { 0.00f, 0.03f, 0.08f, 0.15f, 0.30f, 0.50f, 0.65f, 0.75f, 0.82f, 0.88f, 0.90f };
		const float r[] = { 0.00f, 0.16f, 0.24f, 0.27f, 0.28f, 0.30f, 0.34f, 0.40f, 0.46f, 0.50f, 0.50f };
		const int N = 11;
		const float yCorona = -0.10f;          // la corona queda 10 cm abajo del eje

		auto grosor = [](float hh) { return 0.025f + 0.04f * glm::smoothstep(0.55f, 0.85f, hh); };

		std::vector<glm::vec2> perfil;
		for (int i = 0; i < N; i++)
			perfil.push_back(glm::vec2(r[i], yCorona - h[i]));

		// Interior, del labio hacia la corona
		for (int i = N - 1; i >= 1; i--)
		{
			float ri = (std::max)(0.0f, r[i] - grosor(h[i]));
			float hi = (std::min)(h[i], 0.90f);
			if (i == 1) ri = 0.0f;
			perfil.push_back(glm::vec2(ri, yCorona - (std::max)(hi, 0.06f)));
		}

		mallaCampana = superficieRevolucion(perfil, 40);
	}

	// Badajo: varilla delgada con una bola en la punta
	void crearMallaBadajo()
	{
		std::vector<glm::vec2> perfil;
		perfil.push_back(glm::vec2(0.000f, -0.14f));
		perfil.push_back(glm::vec2(0.030f, -0.15f));
		perfil.push_back(glm::vec2(0.022f, -0.20f));
		perfil.push_back(glm::vec2(0.022f, -0.82f));

		// Bola de radio 0.07 con centro en y = -0.88
		const float cy = -0.88f, rb = 0.07f;
		for (int k = 1; k <= 8; k++)
		{
			float a = glm::radians(60.0f + 120.0f * k / 8.0f);   // de 60° a 180° medido desde arriba
			perfil.push_back(glm::vec2(rb * std::sin(a), cy + rb * std::cos(a)));
		}
		perfil.back().x = 0.0f;

		mallaBadajo = superficieRevolucion(perfil, 16);
	}

	// ---------------- Sonido ----------------

	// Suma de parciales de campana con decaimiento exponencial
	void crearSonido()
	{
		const float f0 = 262.0f;   // "prime" (Do4)
		const float parcial[] = { 0.5f, 1.0f, 1.2f, 1.5f, 2.0f, 2.5f, 3.0f };   // hum, prime, tierce, quint, nominal...
		const float amplitud[] = { 0.55f, 0.45f, 0.35f, 0.18f, 0.40f, 0.12f, 0.08f };
		const float decaimiento[] = { 0.7f, 1.1f, 1.4f, 2.0f, 1.8f, 3.0f, 4.0f };   // 1/s
		const float DOS_PI = 6.28318530718f;

		ondaBase.assign(MUESTRAS, 0.0f);
		float maximo = 0.0f;
		for (int i = 0; i < MUESTRAS; i++)
		{
			float t = (float)i / FRECUENCIA_MUESTREO;
			float ataque = (std::min)(1.0f, t / 0.004f);     // 4 ms de ataque para que no truene
			float s = 0.0f;
			for (int k = 0; k < 7; k++)
				s += amplitud[k] * std::exp(-decaimiento[k] * t) * std::sin(DOS_PI * f0 * parcial[k] * t);
			ondaBase[i] = s * ataque;
			maximo = (std::max)(maximo, std::fabs(ondaBase[i]));
		}
		if (maximo > 0.0f)
			for (float& s : ondaBase) s /= maximo;

		for (int b = 0; b < 2; b++)
			wav[b].assign(44 + MUESTRAS * 2, 0);
	}

	// Escribe un archivo WAV (PCM 16 bits mono) en memoria
	static void escribirCabecera(std::vector<uint8_t>& w, int muestras)
	{
		auto u32 = [&](size_t pos, uint32_t v) { std::memcpy(&w[pos], &v, 4); };
		auto u16 = [&](size_t pos, uint16_t v) { std::memcpy(&w[pos], &v, 2); };
		std::memcpy(&w[0], "RIFF", 4);  u32(4, 36 + muestras * 2);
		std::memcpy(&w[8], "WAVEfmt ", 8);
		u32(16, 16); u16(20, 1); u16(22, 1);
		u32(24, FRECUENCIA_MUESTREO); u32(28, FRECUENCIA_MUESTREO * 2);
		u16(32, 2); u16(34, 16);
		std::memcpy(&w[36], "data", 4); u32(40, muestras * 2);
	}

	// Reproduce el golpe; el volumen depende de la fuerza y de la distancia
	void tocar(float velocidadImpacto, glm::vec3 posJugador)
	{
		float distancia = glm::length(posJugador - pivote);
		float porDistancia = glm::clamp(1.0f - distancia / 70.0f, 0.0f, 1.0f);
		float porFuerza = glm::clamp(velocidadImpacto / 1.2f, 0.25f, 1.0f);
		float volumen = porDistancia * porDistancia * porFuerza * 0.9f;
		if (volumen < 0.02f) return;

		bufferActual = 1 - bufferActual;
		std::vector<uint8_t>& w = wav[bufferActual];
		escribirCabecera(w, MUESTRAS);
		for (int i = 0; i < MUESTRAS; i++)
		{
			int16_t v = (int16_t)(ondaBase[i] * volumen * 32000.0f);
			std::memcpy(&w[44 + i * 2], &v, 2);
		}
		PlaySoundA((LPCSTR)w.data(), NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
	}
};
