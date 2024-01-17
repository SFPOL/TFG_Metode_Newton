#pragma once
#include "Shader.h";

class Material
{
private:
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;
	float opacity;
	GLint diffuseTex;
	GLint specularTex;

public:
	Material(
		glm::vec3 ambient,
		glm::vec3 diffuse,
		glm::vec3 specular,
		float shininess,
		float opacity,
		GLint diffuseTex,
		GLint specularTex
	)
	{
		this->ambient = ambient;
		this->diffuse = diffuse;
		this->specular = specular;
		this->shininess = shininess;
		this->opacity = opacity;
		this->diffuseTex = diffuseTex;
		this->specularTex = specularTex;
	};
	~Material() {};
	void SentToShader(Shader& program)
	{
		program.setVec3f(this->ambient, "material.ambient");
		program.setVec3f(this->diffuse, "material.diffuse");
		program.setVec3f(this->specular, "material.specular");
		program.set1f(this->shininess, "material.shininess");
		program.set1f(this->opacity, "material.opacity");
		program.set1i(this->diffuseTex, "material.diffuseTex");
		program.set1i(this->specularTex, "material.specularTex");
	};

	void setAmbient(glm::vec3 ambient)
	{
		this->ambient = ambient;
	};

	void setDiffuse(glm::vec3 diffuse)
	{
		this->diffuse = diffuse;
	};

	void setSpecular(glm::vec3 specular)
	{
		this->specular = specular;
	};

	void setShininess(float shininess)
	{
		this->shininess = shininess;
	};

	void setOpacity(float opacity)
	{
		this->opacity = opacity;
	};

	void setDiffuseTex(GLint diffuseTex)
	{
		this->diffuseTex = diffuseTex;
	};

	void setSpecularTex(GLint specularTex)
	{
		this->specularTex = specularTex;
	};

	glm::vec3* getAmbient()
	{
		return &this->ambient;
	};

	glm::vec3* getDiffuse()
	{
		return &this->diffuse;
	};

	glm::vec3* getSpecular()
	{
		return &this->specular;
	};

	float* getShininess()
	{
		return &this->shininess;
	};

	float getOpacity()
	{
		return this->opacity;
	};

	GLint getDiffuseTex()
	{
		return this->diffuseTex;
	};

	GLint getSpecularTex()
	{
		return this->specularTex;
	};

};

