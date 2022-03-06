// Created by Modar Nasser on 06/03/2022.

#include "Camera2D.hpp"

#include <cmath>
#include <iostream>


Camera2D::Camera2D(const glm::vec2& size) : m_size(size)
{}

void Camera2D::move(float x, float y) {
    m_transform.x -= x;
    m_transform.y -= y;
}

void Camera2D::zoom(float z) {
    m_transform.z *= z;
}

void Camera2D::centerOn(float x, float y) {
    m_transform.x = -x;
    m_transform.y = -y;
}

template <typename T>
constexpr T pow(T n, unsigned p) {
    return p == 0 ? 1 : n * pow(n, p-1);
}

template <unsigned precision>
constexpr float round(float val) {
    return std::floor(val * pow(10, precision)) / pow(10, precision);
}

glm::vec3 Camera2D::getTransform() {
    return {
        round<3>(m_transform.x / m_size.x),
        round<3>(m_transform.y / m_size.y),
        round<3>(m_transform.z)
    };
}
