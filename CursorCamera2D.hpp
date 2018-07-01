//----------------------------------------------------------------------------------------
//
//	Copyright (C) 2018 SKNJPN
//
//	Licensed under the MIT License.
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files(the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions :
//	
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//	
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.
//
//----------------------------------------------------------------------------------------

// version 1.00

# pragma once
# include <Siv3D.hpp>

enum class CameraSettingMode
{
	BaseOnly,
	TargetOnly,
	BaseAndTarget,
};

class CursorCamera2D
{
	Vec2 m_center = Window::BaseSize() * 0.5;
	double m_magnification = 1.0;

	Vec2 m_targetCenter = Window::BaseSize() * 0.5;
	double m_targetMagnification = 1.0;

	double m_movingSensitivity = 0.02;
	double m_magnifyingSensitivity = 0.1;
	double m_followingSpeed = 0.25;

	double m_minMagnification = 1.0;
	double m_maxMagnification = 8.0;
	RectF m_restrictedRect = Window::BaseClientRect();

	std::array<std::function<bool()>, 4> m_controls =
	{
		[]() { return KeyW.pressed() || Cursor::Pos().y <= 0; },
		[]() { return KeyA.pressed() || Cursor::Pos().x <= 0; },
		[]() { return KeyS.pressed() || Cursor::Pos().y >= Window::Size().y - 1; },
		[]() { return KeyD.pressed() || Cursor::Pos().x >= Window::Size().x - 1; },
	};

	void magnify()
	{
		const auto delta = 1.0 + m_magnifyingSensitivity * Mouse::Wheel();
		const auto cursorPosition = (Cursor::PosF() - Window::BaseSize() * 0.5) / m_targetMagnification + m_targetCenter;

		m_targetMagnification /= delta;
		m_targetCenter = (m_targetCenter - cursorPosition) * delta + cursorPosition;
	}

	void move()
	{
		if (m_controls[0]()) { m_targetCenter.y -= m_movingSensitivity * Window::BaseSize().y / m_targetMagnification; }
		if (m_controls[1]()) { m_targetCenter.x -= m_movingSensitivity * Window::BaseSize().x / m_targetMagnification; }
		if (m_controls[2]()) { m_targetCenter.y += m_movingSensitivity * Window::BaseSize().y / m_targetMagnification; }
		if (m_controls[3]()) { m_targetCenter.x += m_movingSensitivity * Window::BaseSize().x / m_targetMagnification; }
	}

	void follow()
	{
		m_center = Math::Lerp(m_center, m_targetCenter, m_followingSpeed);
		m_magnification = 1.0 / Math::Lerp(1.0 / m_magnification, 1.0 / m_targetMagnification, m_followingSpeed);
	}

	void restrictMagnification()
	{
		m_magnification = Max(m_magnification, m_minMagnification);
		m_magnification = Min(m_magnification, m_maxMagnification);
		m_magnification = Max({ m_magnification, Window::BaseSize().y / m_restrictedRect.h, Window::BaseSize().x / m_restrictedRect.w });

		m_targetMagnification = Max(m_targetMagnification, m_minMagnification);
		m_targetMagnification = Min(m_targetMagnification, m_maxMagnification);
		m_targetMagnification = Max({ m_targetMagnification, Window::BaseSize().y / m_restrictedRect.h, Window::BaseSize().x / m_restrictedRect.w });
	}

	void restrictRect()
	{
		{
			const auto tl = m_restrictedRect.tl() - getCameraRect().tl();
			const auto br = m_restrictedRect.br() - getCameraRect().br();

			m_center.moveBy(Max(0.0, tl.x) + Min(0.0, br.x), Max(0.0, tl.y) + Min(0.0, br.y));
		}
		{
			const auto tl = m_restrictedRect.tl() - getTargetCameraRect().tl();
			const auto br = m_restrictedRect.br() - getTargetCameraRect().br();

			m_targetCenter.moveBy(Max(0.0, tl.x) + Min(0.0, br.x), Max(0.0, tl.y) + Min(0.0, br.y));
		}
	}

public:
	CursorCamera2D() = default;
	~CursorCamera2D() = default;

	void update()
	{
		magnify();

		restrictMagnification();

		move();

		restrictRect();

		follow();
	}

	[[nodiscard]] Mat3x2 getMat3x2() const { return Mat3x2::Translate(-m_center).scaled(m_magnification).translated(Window::BaseSize() * 0.5); }
	[[nodiscard]] Transformer2D createTransformer() const { return Transformer2D(getMat3x2(), true, Transformer2D::Target::PushCamera); }

	[[nodiscard]] RectF getCameraRect() const { return RectF(Window::BaseSize() / m_magnification).setCenter(m_center); }
	[[nodiscard]] RectF getTargetCameraRect() const { return RectF(Window::BaseSize() / m_targetMagnification).setCenter(m_targetCenter); }

	void setCameraRect(const RectF& cameraRect, CameraSettingMode settingMode = CameraSettingMode::TargetOnly)
	{
		setCameraRect(cameraRect.center(), Max(Window::BaseWidth() / cameraRect.w, Window::BaseHeight() / cameraRect.h), settingMode);
	}

	void setCameraRect(const Vec2& center, double magnification, CameraSettingMode settingMode = CameraSettingMode::TargetOnly)
	{
		switch (settingMode)
		{
		case CameraSettingMode::BaseOnly:
			m_center = center;
			m_magnification = magnification;
			break;

		case CameraSettingMode::TargetOnly:
			m_targetCenter = center;
			m_targetMagnification = magnification;
			break;

		case CameraSettingMode::BaseAndTarget:
			m_center = center;
			m_magnification = magnification;
			m_targetCenter = center;
			m_targetMagnification = magnification;
			break;

		default:
			break;
		}
	}

	void setSensitivity(double magnifyingSensitivity, double movingSensitivity, double followingSpeed) noexcept
	{
		m_followingSpeed = followingSpeed;
		m_magnifyingSensitivity = magnifyingSensitivity;
		m_movingSensitivity = movingSensitivity;
	}

	void setControls(const std::array<std::function<bool()>, 4>& controls) noexcept { m_controls = controls; }

	void setRestrictedRect(const RectF& restrictedRect) noexcept { m_restrictedRect = restrictedRect; }
	void setMinMagnification(double minMagnification) noexcept { m_minMagnification = minMagnification; }
	void setMaxMagnification(double maxMagnification) noexcept { m_maxMagnification = maxMagnification; }
};

/* example
# include <Siv3D.hpp> // OpenSiv3D v0.2.6
# include "CursorCamera2D.hpp"

void Main()
{
	Graphics::SetBackground(ColorF(0.8, 0.9, 1.0));

	const Font font(50);

	const Texture textureCat(Emoji(U"🐈"), TextureDesc::Mipped);

	CursorCamera2D cursorCamera2D;

	while (System::Update())
	{
		cursorCamera2D.update();

		{
			const auto t = cursorCamera2D.createTransformer();

			font(U"Hello, Siv3D!🐣").drawAt(Window::Center(), Palette::Black);

			font(Cursor::Pos()).draw(20, 400, ColorF(0.6));

			textureCat.resized(80).draw(540, 380);

			Circle(Cursor::Pos(), 60).draw(ColorF(1, 0, 0, 0.5));
		}
	}
}
*/