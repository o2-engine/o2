#include "o2/stdafx.h"
#include "Input.h"

#include "o2/Utils/System/Time/Time.h"
#include "o2/Config/ProjectConfig.h"
#include "o2/Utils/Debug/Profiling/SimpleProfiler.h"

namespace o2
{
    DECLARE_SINGLETON(Input);

    Input::Input(RefCounter* refCounter):
        Singleton<Input>(refCounter)
    {
        if (GetDeviceType() == DeviceType::PC)
        {
            mCursors.Add(Cursor());
            mCursors.Last().isPressed = false;
        }
    }

    bool Input::IsKeyPressed(KeyboardKey key) const
    {
        for (auto& ikey : mPressedKeys)
        {
            if (ikey.keyCode == key)
                return true;
        }

        return false;
    }

    bool Input::IsKeyDown(KeyboardKey key) const
    {
        for (auto& ikey : mDownKeys)
        {
            if (ikey.keyCode == key)
                return true;
        }

        return IsKeyPressed(key);
    }

    bool Input::IsKeyReleased(KeyboardKey key) const
    {
        for (auto& ikey : mReleasedKeys)
        {
            if (ikey.keyCode == key)
                return true;
        }

        return false;
    }

    bool Input::IsKeyRepeating(KeyboardKey key, float delay /*= 0.1f*/, float beginDelay /*= 0.5f*/) const
    {
        float time = GetKeyPressingTime(key) - beginDelay;
        float repeatTime = Math::Max(floor(time/delay)*delay, 0.0f);

        return time - o2Time.GetDeltaTime() < repeatTime && time > repeatTime;
    }

    float Input::GetKeyPressingTime(KeyboardKey key) const
    {
        for (auto& ikey : mDownKeys)
        {
            if (ikey.keyCode == key)
                return ikey.pressedTime;
        }

        return 0;
    }

    Vec2F Input::GetCursorPos(CursorId id /*= 0*/) const
    {
        for (auto& cursor : mCursors)
        {
            if (cursor.id == id)
                return cursor.position;
        }

        if (id == 0)
            return mMainCursorPos;

        return Vec2F();
    }

    bool Input::IsCursorPressed(CursorId id /*= 0*/) const
    {
        for (auto& cursor : mCursors)
        {
            if (cursor.id == id && cursor.pressedTime < FLT_EPSILON && cursor.isPressed)
                return true;
        }

        return false;
    }

    bool Input::IsCursorDown(CursorId id /*= 0*/) const
    {
        for (auto& cursor : mCursors)
        {
            if (cursor.id == id && cursor.isPressed)
                return true;
        }

        return false;
    }

    bool Input::IsCursorReleased(CursorId id /*= 0*/) const
    {
        for (auto& cursor : mReleasedCursors)
        {
            if (cursor.id == id)
                return true;
        }

        return false;
    }

    float Input::GetCursorPressingTime(CursorId id /*= 0*/) const
    {
        for (auto& cursor : mCursors)
        {
            if (cursor.id == id)
                return cursor.pressedTime;
        }

        return 0;
    }

    Vec2F Input::GetCursorDelta(CursorId id /*= 0*/) const
    {
        for (auto& cursor : mCursors)
        {
            if (cursor.id == id)
            {
                return cursor.delta;
            }
        }

        if (id == 0)
            return mMainCursorDelta;

        return Vec2F();
    }

    bool Input::IsRightMousePressed() const
    {
        return IsKeyPressed((KeyboardKey)-1);
    }

    bool Input::IsRightMouseDown() const
    {
        return IsKeyDown((KeyboardKey)-1);
    }

    bool Input::IsRightMouseReleased() const
    {
        return IsKeyReleased((KeyboardKey)-1);
    }

    float Input::GetRightMousePressedTime() const
    {
        return GetKeyPressingTime((KeyboardKey)-1);
    }

    bool Input::IsMiddleMousePressed() const
    {
        return IsKeyPressed((KeyboardKey)-2);
    }

    bool Input::IsMiddleMouseDown() const
    {
        return IsKeyDown((KeyboardKey)-2);
    }

    bool Input::IsMiddleMouseReleased() const
    {
        return IsKeyReleased((KeyboardKey)-2);
    }

    float Input::GetMiddleMousePressedTime() const
    {
        return GetKeyPressingTime((KeyboardKey)-2);
    }

    float Input::GetMouseWheelDelta() const
    {
        return mMouseWheelDelta;
    }

    Input::Cursor* Input::GetCursor(CursorId id)
    {
        for (auto& cursor : mCursors)
            if (cursor.id == id)
                return &cursor;

        static Cursor dummy;
        return &dummy;
    }

    const Vector<Input::Cursor>& Input::GetCursors() const
    {
        return mCursors;
    }

    const Vector<Input::Cursor>& Input::GetReleasedCursors() const
    {
        return mReleasedCursors;
    }

    Vector<Input::Key> const& Input::GetPressedKeys() const
    {
        return mPressedKeys;
    }

    Vector<Input::Key> const& Input::GetDownKeys() const
    {
        return mDownKeys;
    }

    Vector<Input::Key> const& Input::GetReleasedKeys() const
    {
        return mReleasedKeys;
    }

    void Input::PreUpdate()
    {
        PROFILE_SAMPLE_FUNC();

        auto inputQueue = mInputQueue;
        for (auto& msg : inputQueue)
        {
            if (msg->Apply())
                mInputQueue.Remove(msg);
        }
    }

    void Input::OnKeyPressed(KeyboardKey key)
    {
        //o2Debug.Log("OnKeyPressed: %d", key);

        auto msg = mmake<InputKeyPressedMsg>();
        msg->key = key;
        mInputQueue.Add(msg);
    }

    void Input::OnKeyReleased(KeyboardKey key)
    {
        //o2Debug.Log("OnKeyReleased: %d", key);
        
        auto msg = mmake<InputKeyReleasedMsg>();
        msg->key = key;
        mInputQueue.Add(msg);
    }

    void Input::OnCursorPressed(const Vec2F& pos, CursorId id /*= 0*/)
    {
        auto msg = mmake<InputCursorPressedMsg>();
        msg->position = pos;
        msg->id = id;
        mInputQueue.Add(msg);
    }

    void Input::OnCursorReleased(CursorId id /*= 0*/)
    {
        auto msg = mmake<InputCursorReleasedMsg>();
        msg->id = id;
        mInputQueue.Add(msg);
    }

    void Input::OnCursorMoved(const Vec2F& pos, CursorId id /*= 0*/, bool withDelta /*= true*/)
    {
        auto msg = mmake<InputCursorMovedMsg>();
        msg->position = pos;
        msg->id = id;
        mInputQueue.Add(msg);
    }

    void Input::Update(float dt)
    {
        PROFILE_SAMPLE_FUNC();

        mDownKeys.Add(mPressedKeys);
        mPressedKeys.Clear();
        mReleasedKeys.Clear();

        for (Key& ikey : mDownKeys)
            ikey.pressedTime += dt;

        for (Cursor& cursor : mCursors)
        {
            cursor.pressedTime += dt;
            cursor.delta = Vec2F();
        }

        mMainCursorDelta = Vec2F();

        mReleasedCursors.Clear();

        mMouseWheelDelta = 0;
    }

    bool Input::IsMainCursorPressed()
    {
        return IsCursorPressed();
    }

    bool Input::IsMainCursorDown()
    {
        return IsCursorDown();
    }

    bool Input::IsMainCursorReleased()
    {
        return IsCursorReleased();
    }

    Vec2F Input::GetMainCursorPos()
    {
        return GetCursorPos();
    }

    Vec2F Input::GetMainCursorDelta()
    {
        return GetCursorDelta();
    }

    void Input::OnKeyPressedMsgApply(KeyboardKey key)
    {
        if (IsKeyDown(key) || IsKeyPressed(key))
            return;

        mPressedKeys.Add(Key(key));
    }

    bool Input::OnKeyReleasedMsgApply(KeyboardKey key)
    {
        for (auto& ikey : mPressedKeys)
        {
            if (ikey.keyCode == key)
                return false;
        }
        
        for (auto& ikey : mDownKeys)
        {
            if (ikey.keyCode == key)
            {
                mDownKeys.Remove(ikey);
                mReleasedKeys.Add(Key(key));
                return true;
            }
        }
        
        return true;
    }

    void Input::OnCursorPressedMsgApply(const Vec2F& pos, CursorId id /*= 0*/)
    {
        if (id == 0 && GetDeviceType() == DeviceType::PC)
        {
            mCursors[0].position = pos;
            mCursors[0].isPressed = true;
            mCursors[0].pressedTime = 0.0f;
        }
        else mCursors.Add(Cursor(pos, id));
    }

    void Input::OnCursorMovedMsgApply(const Vec2F& pos, CursorId id /*= 0*/, bool withDelta /*= true*/)
    {
        for (Cursor& cursor : mCursors)
        {
            if (cursor.id == id)
            {
                if (withDelta)
                    cursor.delta += pos - cursor.position;

                cursor.position = pos;
                break;
            }
        }

        if (id == 0)
        {
            if (withDelta)
                mMainCursorDelta += pos - mMainCursorPos;

            mMainCursorPos = pos;
        }
    }

    void Input::OnCursorReleasedMsgApply(CursorId id /*= 0*/)
    {
        Cursor releasedCursor(Vec2F(), -100);
        for (auto& cursor : mCursors)
        {
            if (cursor.id == id)
            {
                releasedCursor = cursor;

                if (id == 0 && GetDeviceType() == DeviceType::PC)
                    cursor.isPressed = false;
                else
                    mCursors.Remove(cursor);

                break;
            }
        }

        mReleasedCursors.Add(releasedCursor);
    }

    void Input::OnMouseWheelMsgApply(float delta)
    {
        mMouseWheelDelta += delta;
    }

    void Input::OnAltCursorPressed(const Vec2F& pos)
    {
        OnKeyPressed(-1);
        OnCursorMoved(pos);
    }

    void Input::OnAltCursorReleased()
    {
        OnKeyReleased(-1);
    }

    void Input::OnAlt2CursorPressed(const Vec2F& pos)
    {
        OnKeyPressed(-2);
        OnCursorMoved(pos);
    }

    void Input::OnAlt2CursorReleased()
    {
        OnKeyReleased(-2);
    }

    void Input::OnMouseWheel(float delta)
    {
        auto msg = mmake<InputMouseWheelMsg>();
        msg->delta = delta;
        mInputQueue.Add(msg);
    }


    bool Input::Cursor::operator==(const Cursor& other) const
    {
        return Math::Equals(position, other.position) && Math::Equals(delta, other.delta) && id == other.id &&
            Math::Equals(pressedTime, other.pressedTime);
    }

    Input::Cursor::Cursor(const Vec2F& position /*= Vec2F()*/, CursorId id /*= 0*/):
        position(position), delta(), id(id), pressedTime(0), isPressed(true)
    {}


    bool Input::Key::operator==(const Key& other) const
    {
        return keyCode == other.keyCode && Math::Equals(other.pressedTime, pressedTime);
    }

    Input::Key::Key(KeyboardKey key /*= 0*/):keyCode(key), pressedTime(0)
    {}

    bool Input::Key::operator==(KeyboardKey key) const
    {
        return keyCode == key;
    }

    bool Input::InputCursorPressedMsg::Apply()
    {
        o2Input.OnCursorPressedMsgApply(position, id);
        return true;
    }

    bool Input::InputCursorMovedMsg::Apply()
    {
        o2Input.OnCursorMovedMsgApply(position, id);
        return true;
    }

    bool Input::InputCursorReleasedMsg::Apply()
    {
        o2Input.OnCursorReleasedMsgApply(id);
        return true;
    }

    bool Input::InputKeyPressedMsg::Apply()
    {
        o2Input.OnKeyPressedMsgApply(key);

    #if defined PLATFORM_MAC
        if (key == VK_COMMAND)
            o2Input.OnKeyPressedMsgApply(VK_CTRL_CMD);
    #elif defined PLATFORM_WINDOWS || defined PLATFORM_LINUX
        if (key == VK_CONTROL)
            o2Input.OnKeyPressedMsgApply(VK_CTRL_CMD);
    #endif

        return true;
    }

    bool Input::InputKeyReleasedMsg::Apply()
    {
        bool result = o2Input.OnKeyReleasedMsgApply(key);

    #if defined PLATFORM_MAC
        if (key == VK_COMMAND)
            o2Input.OnKeyReleasedMsgApply(VK_CTRL_CMD);
    #elif defined PLATFORM_WINDOWS || defined PLATFORM_LINUX
        if (key == VK_CONTROL)
            o2Input.OnKeyReleasedMsgApply(VK_CTRL_CMD);
    #endif

        return result;
    }

    bool Input::InputMouseWheelMsg::Apply()
    {
        o2Input.OnMouseWheelMsgApply(delta);
        return true;
    }

}
