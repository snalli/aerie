#include "common/interval_tree.h"
#include <gtest/gtest.h>

class SimpleInterval : public Interval
{
  public:
    SimpleInterval(const int low, const int high) : low_(low), high_(high)
    {
    }

    int GetLowPoint() const
    {
        return low_;
    }
    int GetHighPoint() const
    {
        return high_;
    }
    IntervalTreeNode* GetNode()
    {
        return node_;
    }
    void SetNode(IntervalTreeNode* node)
    {
        node_ = node;
    }

  protected:
    int low_;
    int high_;
    IntervalTreeNode* node_;
};

// Suite: SuiteIntervalTree
    TEST(SuiteIntervalTree, TestInsert1)
    {
        IntervalTree* tree = new IntervalTree;
        SimpleInterval* result_interval;
        SimpleInterval* interval;

        interval = new SimpleInterval(10, 20);
        tree->Insert(interval);

        result_interval = static_cast<SimpleInterval*>(tree->LeftmostOverlap(10, 10));
        EXPECT_TRUE(result_interval->GetLowPoint() == 10);
        EXPECT_TRUE(result_interval->GetHighPoint() == 20);
    }

    TEST(SuiteIntervalTree, TestInsert2)
    {
        IntervalTree* tree = new IntervalTree;
        SimpleInterval* result_interval;
        SimpleInterval* interval;

        interval = new SimpleInterval(10, 30);
        tree->Insert(interval);
        interval = new SimpleInterval(50, 100);
        tree->Insert(interval);
        interval = new SimpleInterval(800, 1000);
        tree->Insert(interval);
        interval = new SimpleInterval(180, 220);
        tree->Insert(interval);

        result_interval = static_cast<SimpleInterval*>(tree->LeftmostOverlap(10, 10));
        EXPECT_TRUE(result_interval->GetLowPoint() == 10);
        EXPECT_TRUE(result_interval->GetHighPoint() == 30);

        result_interval = static_cast<SimpleInterval*>(tree->LeftmostOverlap(40, 90));
        EXPECT_TRUE(result_interval->GetLowPoint() == 50);
        EXPECT_TRUE(result_interval->GetHighPoint() == 100);
    }

    TEST(SuiteIntervalTree, TestInsert3)
    {
        IntervalTree* tree = new IntervalTree;
        SimpleInterval* result_interval;
        SimpleInterval* interval;

        interval = new SimpleInterval(10, 30);
        tree->Insert(interval);
        interval = new SimpleInterval(50, 80);
        tree->Insert(interval);
        interval = new SimpleInterval(800, 1000);
        tree->Insert(interval);
        interval = new SimpleInterval(180, 220);
        tree->Insert(interval);
        interval = new SimpleInterval(85, 95);
        tree->Insert(interval);

        result_interval = static_cast<SimpleInterval*>(tree->LeftmostOverlap(20, 90));
        EXPECT_TRUE(result_interval->GetLowPoint() == 10);
        EXPECT_TRUE(result_interval->GetHighPoint() == 30);

        result_interval = static_cast<SimpleInterval*>(tree->LeftmostOverlap(31, 90));
        EXPECT_TRUE(result_interval->GetLowPoint() == 50);
        EXPECT_TRUE(result_interval->GetHighPoint() == 80);

        result_interval = static_cast<SimpleInterval*>(tree->LeftmostOverlap(81, 90));
        EXPECT_TRUE(result_interval->GetLowPoint() == 85);
        EXPECT_TRUE(result_interval->GetHighPoint() == 95);
    }
