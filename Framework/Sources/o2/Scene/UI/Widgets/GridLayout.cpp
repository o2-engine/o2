#include "o2/stdafx.h"
#include "GridLayout.h"

#include "o2/Scene/UI/WidgetLayout.h"

namespace o2
{
    GridLayout::GridLayout(RefCounter* refCounter):
        Widget(refCounter)
    {
        SetLayoutDirty();
    }

    GridLayout::GridLayout(RefCounter* refCounter, const GridLayout& other):
        Widget(refCounter, other), mBaseCorner(other.mBaseCorner), mSpacing(other.mSpacing), mBorder(other.mBorder),
        mFitByChildren(other.mFitByChildren), mCellSize(other.mCellSize), mArrangeAxis(other.mArrangeAxis),
        mArrangeAxisMaxCells(other.mArrangeAxisMaxCells)
    {
        RetargetStatesAnimations();
        SetLayoutDirty();
    }

    GridLayout::~GridLayout()
    {}

    GridLayout& GridLayout::operator=(const GridLayout& other)
    {
        mBaseCorner = other.mBaseCorner;
        mSpacing = other.mSpacing;
        mBorder = other.mBorder;
        mCellSize = other.mCellSize;
        mArrangeAxis = other.mArrangeAxis;
        mArrangeAxisMaxCells = other.mArrangeAxisMaxCells;

        Widget::operator=(other);

        RetargetStatesAnimations();
        SetLayoutDirty();

        return *this;
    }

    void GridLayout::SetBaseCorner(BaseCorner baseCorner)
    {
        mBaseCorner = baseCorner;
        layout->SetDirty();
    }

    BaseCorner GridLayout::GetBaseCorner() const
    {
        return mBaseCorner;
    }

    void GridLayout::SetSpacing(float spacing)
    {
        mSpacing = spacing;
        layout->SetDirty();
    }

    float GridLayout::GetSpacing() const
    {
        return mSpacing;
    }

    void GridLayout::SetBorder(const RectF& border)
    {
        mBorder = border;
        layout->SetDirty();
    }

    RectF GridLayout::GetBorder() const
    {
        return mBorder;
    }

    void GridLayout::SetBorderLeft(float value)
    {
        mBorder.left = value;
        layout->SetDirty();
    }

    float GridLayout::GetBorderLeft() const
    {
        return mBorder.left;
    }

    void GridLayout::SetBorderRight(float value)
    {
        mBorder.right = value;
        layout->SetDirty();
    }

    float GridLayout::GetBorderRight() const
    {
        return mBorder.right;
    }

    void GridLayout::SetBorderTop(float value)
    {
        mBorder.top = value;
        layout->SetDirty();
    }

    float GridLayout::GetBorderTop() const
    {
        return mBorder.top;
    }

    void GridLayout::SetBorderBottom(float value)
    {
        mBorder.bottom = value;
        layout->SetDirty();
    }

    float GridLayout::GetBorderBottom() const
    {
        return mBorder.bottom;
    }

    void GridLayout::SetCellSize(const Vec2F& size)
    {
        mCellSize = size;
        SetLayoutDirty();
    }

    Vec2F GridLayout::GetCellSize() const
    {
        return mCellSize;
    }

    void GridLayout::SetArrangeAxis(TwoDirection type)
    {
        mArrangeAxis = type;
        SetLayoutDirty();
    }

    TwoDirection GridLayout::GetArrangeAxis() const
    {
        return mArrangeAxis;
    }

    void GridLayout::SetArrangeAxisMaxCells(int count)
    {
        if (mArrangeAxisMaxCells == count)
            return;

        mArrangeAxisMaxCells = count;
        SetLayoutDirty();
    }

    int GridLayout::GetArrangeAxisMaxCells() const
    {
        return mArrangeAxisMaxCells;
    }

    void GridLayout::SetFitByChildren(bool fit)
    {
        mFitByChildren = fit;
        layout->SetDirty();
    }

    bool GridLayout::IsFittingByChildren() const
    {
        return mFitByChildren;
    }

    void GridLayout::UpdateSelfTransform()
{
        if (mFitByChildren)
            ExpandSizeByChilds();

        Widget::UpdateSelfTransform();

        RearrangeChilds();
    }

    String GridLayout::GetCreateMenuGroup()
    {
        return "Layout";
    }

    void GridLayout::OnChildAdded(const Ref<Widget>& child)
    {
        child->GetLayoutData().drivenByParent = true;
    }

    void GridLayout::OnChildRemoved(const Ref<Widget>& child)
    {
        child->GetLayoutData().drivenByParent = false;
    }

    void GridLayout::RearrangeChilds()
    {
        switch (mBaseCorner)
        {
            case BaseCorner::LeftTop: ArrangeFromLeftTop(); break;
            case BaseCorner::Top: ArrangeFromTop(); break;
            case BaseCorner::RightTop: ArrangeFromRightTop(); break;
            case BaseCorner::Left: ArrangeFromLeft(); break;
            case BaseCorner::Center: ArrangeFromCenter(); break;
            case BaseCorner::Right: ArrangeFromRight(); break;
            case BaseCorner::LeftBottom: ArrangeFromLeftBottom(); break;
            case BaseCorner::Bottom: ArrangeFromBottom(); break;
            case BaseCorner::RightBottom: ArrangeFromRightBottom(); break;
        }
    }

    void GridLayout::ArrangeFromLeftTop()
    {
        int cellsCount = mChildren.Count();

        if (mArrangeAxis == TwoDirection::Horizontal)
        {
            Vec2F anchor(0.0f, 1.0f);
            Vec2F cellPos(mBorder.left, -mBorder.top);

            int rowIdx = 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (rowIdx == mArrangeAxisMaxCells)
                {
                    cellPos.x = mBorder.left;
                    cellPos.y -= mCellSize.y + mSpacing;
                    rowIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = Vec2F(cellPos.x, cellPos.y - mCellSize.y);
                child->GetLayoutData().offsetMax = Vec2F(cellPos.x + mCellSize.x, cellPos.y);

                cellPos.x += mCellSize.x + mSpacing;

                rowIdx++;
            }
        }
        else
        {
            Vec2F anchor(0.0f, 1.0f);
            Vec2F cellPos(mBorder.left, -mBorder.top);

            int colIdx = 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (colIdx == mArrangeAxisMaxCells)
                {
                    cellPos.x += mCellSize.y + mSpacing;
                    cellPos.y = -mBorder.top;
                    colIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = Vec2F(cellPos.x, cellPos.y - mCellSize.y);
                child->GetLayoutData().offsetMax = Vec2F(cellPos.x + mCellSize.x, cellPos.y);

                cellPos.y -= mCellSize.y + mSpacing;

                colIdx++;
            }
        }
    }

    void GridLayout::ArrangeFromTop()
    {
        int cellsCount = mChildren.Count();
        int rem = cellsCount % mArrangeAxisMaxCells;

        if (mArrangeAxis == TwoDirection::Horizontal)
        {
            int rowCells = Math::Min(cellsCount, mArrangeAxisMaxCells);

            Vec2F anchor(0.5f, 1.0f);
            Vec2F cellPos(-((float)rowCells*mCellSize.x + (float)(rowCells - 1)*mSpacing)*0.5f, -mBorder.top);

            int rowIdx = 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (rowIdx == mArrangeAxisMaxCells)
                {
                    rowCells = Math::Min(cellsCount - i, mArrangeAxisMaxCells);
                    cellPos.x = -((float)rowCells*mCellSize.x + (float)(rowCells - 1)*mSpacing)*0.5f;
                    cellPos.y -= mCellSize.y + mSpacing;
                    rowIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = Vec2F(cellPos.x, cellPos.y - mCellSize.y);
                child->GetLayoutData().offsetMax = Vec2F(cellPos.x + mCellSize.x, cellPos.y);

                cellPos.x += mCellSize.x + mSpacing;

                rowIdx++;
            }
        }
        else
        {
            int cols = cellsCount / mArrangeAxisMaxCells;
            if (rem != 0)
            {
                if (rem > 1) cols += 2;
                else cols++;
            }

            Vec2F anchor(0.5f, 1.0f);
            Vec2F cellPos(-((float)cols*mCellSize.x + (float)(cols - 1)*mSpacing)*0.5f, -mBorder.top);

            int upColCells = rem - rem / 2;

            int colIdx = upColCells > 0 ? mArrangeAxisMaxCells - upColCells : 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (colIdx == mArrangeAxisMaxCells)
                {
                    cellPos.x += mCellSize.x + mSpacing;
                    cellPos.y = -mBorder.top;
                    colIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = Vec2F(cellPos.x, cellPos.y - mCellSize.y);
                child->GetLayoutData().offsetMax = Vec2F(cellPos.x + mCellSize.x, cellPos.y);

                cellPos.y -= mCellSize.y + mSpacing;

                colIdx++;
            }
        }
    }

    void GridLayout::ArrangeFromRightTop()
    {
        int cellsCount = mChildren.Count();

        if (mArrangeAxis == TwoDirection::Horizontal)
        {
            Vec2F anchor(1.0f, 1.0f);
            Vec2F cellPos(-mBorder.right, -mBorder.top);

            int rowIdx = 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (rowIdx == mArrangeAxisMaxCells)
                {
                    cellPos.x = -mBorder.right;
                    cellPos.y -= mCellSize.y + mSpacing;
                    rowIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = cellPos - mCellSize;
                child->GetLayoutData().offsetMax = cellPos;

                cellPos.x -= mCellSize.x + mSpacing;

                rowIdx++;
            }
        }
        else
        {
            Vec2F anchor(1.0f, 1.0f);
            Vec2F cellPos(-mBorder.right, -mBorder.top);

            int colIdx = 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (colIdx == mArrangeAxisMaxCells)
                {
                    cellPos.x -= mCellSize.x + mSpacing;
                    cellPos.y = -mBorder.top;
                    colIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = cellPos - mCellSize;
                child->GetLayoutData().offsetMax = cellPos;

                cellPos.y -= mCellSize.y + mSpacing;

                colIdx++;
            }
        }
    }

    void GridLayout::ArrangeFromLeft()
    {
        int cellsCount = mChildren.Count();
        int rem = cellsCount % mArrangeAxisMaxCells;

        if (mArrangeAxis == TwoDirection::Horizontal)
        {
            int rows = cellsCount / mArrangeAxisMaxCells;
            if (rem != 0)
            {
                if (rem > 1) rows += 2;
                else rows++;
            }

            Vec2F anchor(0.0f, 0.5f);
            Vec2F cellPos(mBorder.left, ((float)rows*mCellSize.y + (float)(rows - 1)*mSpacing)*0.5f);

            int upRowCells = rem - rem / 2;

            int rowIdx = upRowCells > 0 ? mArrangeAxisMaxCells - upRowCells : 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (rowIdx == mArrangeAxisMaxCells)
                {
                    cellPos.x = mBorder.left;
                    cellPos.y -= mCellSize.y + mSpacing;
                    rowIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = Vec2F(cellPos.x, cellPos.y - mCellSize.y);
                child->GetLayoutData().offsetMax = Vec2F(cellPos.x + mCellSize.x, cellPos.y);

                cellPos.x += mCellSize.x + mSpacing;

                rowIdx++;
            }
        }
        else
        {
            int cols = cellsCount / mArrangeAxisMaxCells;
            if (rem != 0) cols++;

            int colCells = Math::Min(cellsCount, mArrangeAxisMaxCells);

            Vec2F anchor(0.0f, 0.5f);
            Vec2F cellPos(mBorder.left, ((float)colCells*mCellSize.y + (float)(colCells - 1)*mSpacing)*0.5f);

            int colIdx = 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (colIdx == mArrangeAxisMaxCells)
                {
                    colCells = Math::Min(cellsCount - i, mArrangeAxisMaxCells);

                    cellPos.x += mCellSize.x + mSpacing;
                    cellPos.y = ((float)colCells*mCellSize.y + (float)(colCells - 1)*mSpacing)*0.5f;
                    colIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = Vec2F(cellPos.x, cellPos.y - mCellSize.y);
                child->GetLayoutData().offsetMax = Vec2F(cellPos.x + mCellSize.x, cellPos.y);

                cellPos.y -= mCellSize.y + mSpacing;

                colIdx++;
            }
        }
    }

    void GridLayout::ArrangeFromCenter()
    {
        int cellsCount = mChildren.Count();
        int rem = cellsCount % mArrangeAxisMaxCells;

        if (mArrangeAxis == TwoDirection::Horizontal)
        {
            int rows = cellsCount / mArrangeAxisMaxCells;
            if (rem != 0)
            {
                if (rem > 1) rows += 2;
                else rows++;
            }

            int upRowCells = rem / 2;
            int rowCells = Math::Min(cellsCount, mArrangeAxisMaxCells);

            Vec2F anchor(0.5f, 0.5f);
            Vec2F cellPos(-((float)rowCells*mCellSize.x + (float)(rowCells - 1)*mSpacing)*0.5f,
                          ((float)rows*mCellSize.y + (float)(rows - 1)*mSpacing)*0.5f);

            int rowIdx = 0;
            if (upRowCells > 0)
            {
                cellPos.x = -((float)upRowCells*mCellSize.x + (float)(upRowCells - 1)*mSpacing)*0.5f;
                rowIdx = mArrangeAxisMaxCells - upRowCells;
            }

            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (rowIdx == mArrangeAxisMaxCells)
                {
                    rowCells = Math::Min(cellsCount - i, mArrangeAxisMaxCells);
                    cellPos.x = -((float)rowCells*mCellSize.x + (float)(rowCells - 1)*mSpacing)*0.5f;
                    cellPos.y -= mCellSize.y + mSpacing;
                    rowIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = Vec2F(cellPos.x, cellPos.y - mCellSize.y);
                child->GetLayoutData().offsetMax = Vec2F(cellPos.x + mCellSize.x, cellPos.y);

                cellPos.x += mCellSize.x + mSpacing;

                rowIdx++;
            }
        }
        else
        {
            int cols = cellsCount / mArrangeAxisMaxCells;
            if (rem != 0)
            {
                if (rem > 1) cols += 2;
                else cols++;
            }

            int upColCells = rem / 2;
            int colCells = Math::Min(cellsCount, mArrangeAxisMaxCells);

            Vec2F anchor(0.5f, 0.5f);
            Vec2F cellPos(-((float)cols*mCellSize.x + (float)(cols - 1)*mSpacing)*0.5f,
                          ((float)colCells*mCellSize.y + (float)(colCells - 1)*mSpacing)*0.5f);

            int colIdx = 0;
            if (upColCells > 0)
            {
                cellPos.y = ((float)upColCells*mCellSize.y + (float)(upColCells - 1)*mSpacing)*0.5f;
                colIdx = mArrangeAxisMaxCells - upColCells;
            }

            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (colIdx == mArrangeAxisMaxCells)
                {
                    colCells = Math::Min(cellsCount - i, mArrangeAxisMaxCells);
                    cellPos.x += mCellSize.x + mSpacing;
                    cellPos.y = ((float)colCells*mCellSize.y + (float)(colCells - 1)*mSpacing)*0.5f;
                    colIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = Vec2F(cellPos.x, cellPos.y - mCellSize.y);
                child->GetLayoutData().offsetMax = Vec2F(cellPos.x + mCellSize.x, cellPos.y);

                cellPos.y -= mCellSize.y + mSpacing;

                colIdx++;
            }
        }
    }

    void GridLayout::ArrangeFromRight()
    {
        int cellsCount = mChildren.Count();
        int rem = cellsCount % mArrangeAxisMaxCells;

        if (mArrangeAxis == TwoDirection::Horizontal)
        {
            int rows = cellsCount / mArrangeAxisMaxCells;
            if (rem != 0)
            {
                if (rem > 1) rows += 2;
                else rows++;
            }

            Vec2F anchor(1.0f, 0.5f);
            Vec2F cellPos(-mBorder.right, ((float)rows*mCellSize.y + (float)(rows - 1)*mSpacing)*0.5f);

            int upRowCells = rem - rem / 2;

            int rowIdx = upRowCells > 0 ? mArrangeAxisMaxCells - upRowCells : 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (rowIdx == mArrangeAxisMaxCells)
                {
                    cellPos.x = -mBorder.right;
                    cellPos.y -= mCellSize.y + mSpacing;
                    rowIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = cellPos - mCellSize;
                child->GetLayoutData().offsetMax = cellPos;

                cellPos.x -= mCellSize.x + mSpacing;

                rowIdx++;
            }
        }
        else
        {
            int cols = cellsCount / mArrangeAxisMaxCells;
            if (rem != 0) cols++;

            int colCells = Math::Min(cellsCount, mArrangeAxisMaxCells);

            Vec2F anchor(1.0f, 0.5f);
            Vec2F cellPos(-mBorder.right, ((float)colCells*mCellSize.y + (float)(colCells - 1)*mSpacing)*0.5f);

            int colIdx = 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (colIdx == mArrangeAxisMaxCells)
                {
                    colCells = Math::Min(cellsCount - i, mArrangeAxisMaxCells);

                    cellPos.x -= mCellSize.x + mSpacing;
                    cellPos.y = ((float)colCells*mCellSize.y + (float)(colCells - 1)*mSpacing)*0.5f;
                    colIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = cellPos - mCellSize;
                child->GetLayoutData().offsetMax = cellPos;

                cellPos.y -= mCellSize.y + mSpacing;

                colIdx++;
            }
        }
    }

    void GridLayout::ArrangeFromLeftBottom()
    {
        int cellsCount = mChildren.Count();
        int rem = cellsCount % mArrangeAxisMaxCells;

        if (mArrangeAxis == TwoDirection::Horizontal)
        {
            int rows = cellsCount / mArrangeAxisMaxCells;
            if (rem != 0) rows++;

            Vec2F anchor(0.0f, 0.0f);
            Vec2F cellPos(mBorder.left, mBorder.bottom + (float)rows*mCellSize.y + (float)(rows - 1)*mSpacing);

            int rowIdx = rem != 0 ? mArrangeAxisMaxCells - rem : 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (rowIdx == mArrangeAxisMaxCells)
                {
                    cellPos.x = mBorder.left;
                    cellPos.y -= mCellSize.y + mSpacing;
                    rowIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = Vec2F(cellPos.x, cellPos.y - mCellSize.y);
                child->GetLayoutData().offsetMax = Vec2F(cellPos.x + mCellSize.x, cellPos.y);

                cellPos.x += mCellSize.x + mSpacing;

                rowIdx++;
            }
        }
        else
        {
            Vec2F anchor(0.0f, 0.0f);
            Vec2F cellPos(mBorder.left, mBorder.bottom);

            int colIdx = 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (colIdx == mArrangeAxisMaxCells)
                {
                    cellPos.x += mCellSize.x + mSpacing;
                    cellPos.y = mBorder.bottom;
                    colIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = cellPos;
                child->GetLayoutData().offsetMax = cellPos + mCellSize;

                cellPos.y += mCellSize.y + mSpacing;

                colIdx++;
            }
        }
    }

    void GridLayout::ArrangeFromBottom()
    {
        int cellsCount = mChildren.Count();
        int rem = cellsCount % mArrangeAxisMaxCells;

        if (mArrangeAxis == TwoDirection::Horizontal)
        {
            int rows = cellsCount / mArrangeAxisMaxCells;
            if (rem != 0) rows++;

            int colCells = Math::Min(cellsCount, mArrangeAxisMaxCells);

            Vec2F anchor(0.5f, 0.0f);
            Vec2F cellPos(-((float)colCells*mCellSize.x + (float)(colCells - 1)*mSpacing)*0.5f,
                          mBorder.bottom + (float)rows*mCellSize.y + (float)(rows - 1)*mSpacing);

            int rowIdx = 0;
            if (rem != 0)
            {
                cellPos.x = -((float)rem*mCellSize.x + (float)(rem - 1)*mSpacing)*0.5f;
                rowIdx = mArrangeAxisMaxCells - rem;
            }

            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (rowIdx == mArrangeAxisMaxCells)
                {
                    colCells = Math::Min(cellsCount - i, mArrangeAxisMaxCells);
                    cellPos.x = -((float)colCells*mCellSize.x + (float)(colCells - 1)*mSpacing)*0.5f;
                    cellPos.y -= mCellSize.y + mSpacing;
                    rowIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = Vec2F(cellPos.x, cellPos.y - mCellSize.y);
                child->GetLayoutData().offsetMax = Vec2F(cellPos.x + mCellSize.x, cellPos.y);

                cellPos.x += mCellSize.x + mSpacing;

                rowIdx++;
            }
        }
        else
        {
            int cols = cellsCount / mArrangeAxisMaxCells;
            if (rem != 0)
            {
                if (rem > 1) cols += 2;
                else cols++;
            }

            int colCells = Math::Min(cellsCount, mArrangeAxisMaxCells);

            Vec2F anchor(0.5f, 0.0f);
            Vec2F cellPos(-((float)cols*mCellSize.x + (float)(cols - 1)*mSpacing)*0.5f, mBorder.bottom);

            int upColCells = rem / 2;
            int colIdx = upColCells > 0 ? mArrangeAxisMaxCells - upColCells : 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (colIdx == mArrangeAxisMaxCells)
                {
                    colCells = Math::Min(cellsCount - i, mArrangeAxisMaxCells);
                    cellPos.x += mCellSize.x + mSpacing;
                    cellPos.y = mBorder.bottom;
                    colIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = cellPos;
                child->GetLayoutData().offsetMax = cellPos + mCellSize;

                cellPos.y += mCellSize.y + mSpacing;

                colIdx++;
            }
        }
    }

    void GridLayout::ArrangeFromRightBottom()
    {
        int cellsCount = mChildren.Count();
        int rem = cellsCount % mArrangeAxisMaxCells;

        if (mArrangeAxis == TwoDirection::Horizontal)
        {
            int rows = cellsCount / mArrangeAxisMaxCells;
            if (rem != 0) rows++;

            Vec2F anchor(1.0f, 0.0f);
            Vec2F cellPos(-mBorder.right, mBorder.bottom);

            int rowIdx = 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (rowIdx == mArrangeAxisMaxCells)
                {
                    cellPos.x = -mBorder.right;
                    cellPos.y += mCellSize.y + mSpacing;
                    rowIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = Vec2F(cellPos.x - mCellSize.x, cellPos.y);
                child->GetLayoutData().offsetMax = Vec2F(cellPos.x, cellPos.y + mCellSize.y);

                cellPos.x -= mCellSize.x + mSpacing;

                rowIdx++;
            }
        }
        else
        {
            Vec2F anchor(1.0f, 0.0f);
            Vec2F cellPos(-mBorder.right, mBorder.bottom);

            int colIdx = 0;
            for (int i = 0; i < cellsCount; i++)
            {
                auto child = mChildWidgets[i];

                if (colIdx == mArrangeAxisMaxCells)
                {
                    cellPos.x -= mCellSize.x + mSpacing;
                    cellPos.y = mBorder.bottom;
                    colIdx = 0;
                }

                child->GetLayoutData().anchorMin = anchor;
                child->GetLayoutData().anchorMax = anchor;
                child->GetLayoutData().offsetMin = Vec2F(cellPos.x - mCellSize.x, cellPos.y);
                child->GetLayoutData().offsetMax = Vec2F(cellPos.x, cellPos.y + mCellSize.y);

                cellPos.y += mCellSize.y + mSpacing;

                colIdx++;
            }
        }
    }

    void GridLayout::ExpandSizeByChilds()
    {
        const static Vec2F relativePivots[] = {
            Vec2F(0.0f, 0.5f), // Left
            Vec2F(1.0f, 0.5f), // Right
            Vec2F(0.5f, 1.0f), // Top
            Vec2F(0.5f, 0.0f), // Bottom
            Vec2F(0.5f, 0.5f), // Center
            Vec2F(0.0f, 0.0f), // LeftBottom
            Vec2F(0.0f, 1.0f), // LeftTop
            Vec2F(1.0f, 0.0f), // RightBottom
            Vec2F(1.0f, 1.0f)  // RightTop
        };

        Vec2F relativePivot = relativePivots[(int)mBaseCorner];

        RectF childrenRect;
        if (mChildWidgets.Count() > 0)
            childrenRect = mChildWidgets[0]->GetLayoutData().rectangle;

        for (auto& child : mChildWidgets)
        {
            childrenRect.left   = Math::Min(childrenRect.left, child->GetLayoutData().rectangle.left);
            childrenRect.right  = Math::Max(childrenRect.right, child->GetLayoutData().rectangle.right);
            childrenRect.bottom = Math::Min(childrenRect.bottom, child->GetLayoutData().rectangle.bottom);
            childrenRect.top    = Math::Max(childrenRect.top, child->GetLayoutData().rectangle.top);
        }

        Vec2F szDelta = (childrenRect.Size() + mBorder.LeftBottom() + mBorder.RightTop()) - GetChildrenWorldRect().Size();

        if (szDelta != Vec2F())
        {
            GetLayoutData().offsetMax += szDelta*(Vec2F::One() - relativePivot);
            GetLayoutData().offsetMin -= szDelta*relativePivot;

            layout->SetDirty();
        }
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::GridLayout>);
// --- META ---

DECLARE_CLASS(o2::GridLayout, o2__GridLayout);
// --- END META ---
