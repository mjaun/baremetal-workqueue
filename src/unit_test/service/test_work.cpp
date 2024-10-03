#include <service/unit_test.h>
#include <service/work.h>
#include <util/container_of.h>
#include <functional>
#include <vector>
#include <service/system.h>

SimpleString StringFrom(const std::vector<class fake_work *> &order)
{
    SimpleString ret = "";

    for (auto *item: order) {
        if (!ret.isEmpty()) {
            ret += ", ";
        }

        ret += StringFrom(item);
    }

    return ret;
}

class fake_work {
public:
    explicit fake_work(int32_t priority, std::function<void()> callback = std::function<void()>()) :
        m_work(WORK_INITIALIZER(priority, fake_work_handler)),
        m_callback(std::move(callback))
    {
    }

    work *get()
    {
        return &m_work;
    }

    u64_ms_t last_execution() const
    {
        return m_last_execution;
    }

    static void reset()
    {
        s_execution_order.clear();
    }

    static void check(auto &... items)
    {
        std::vector<fake_work *> expected = {&items...};
        CHECK_EQUAL(expected, s_execution_order);
    }

private:
    work m_work;
    u64_ms_t m_last_execution;
    std::function<void()> m_callback;

    static void fake_work_handler(work *work)
    {
        auto *fake_work = reinterpret_cast<class fake_work *>(work);
        s_execution_order.push_back(fake_work);
        fake_work->m_last_execution = system_uptime_get_ms();

        if (fake_work->m_callback) {
            fake_work->m_callback();
        }
    }

    static std::vector<fake_work *> s_execution_order;
};

std::vector<fake_work *> fake_work::s_execution_order;

TEST_GROUP(work) {
    void setup() override { fake_work::reset(); }
};

TEST(work, submit)
{
    fake_work work1(5);
    fake_work work2(5);
    fake_work work3(5);

    work_submit(work1.get());
    work_submit(work2.get());
    work_submit(work3.get());

    fake_work::check();
    work_run_for(0);
    fake_work::check(work1, work2, work3);
}

TEST(work, submit_priorities)
{
    fake_work work1(1);
    fake_work work2(2);
    fake_work work3(3);
    fake_work work3_2(3);
    fake_work work4(4);

    work_submit(work2.get()); // appended to end
    work_submit(work3.get()); // appended to end
    work_submit(work4.get()); // appended to end
    work_submit(work1.get()); // inserted in front
    work_submit(work3_2.get()); // inserted after work3

    fake_work::check();
    work_run_for(0);
    fake_work::check(work1, work2, work3, work3_2, work4);
}

TEST(work, schedule_after)
{
    auto test_start = system_uptime_get_ms();

    fake_work work1(0);
    fake_work work2(0);
    fake_work work3(0);

    work_schedule_after(work3.get(), 3000);
    work_schedule_after(work1.get(), 1000);
    work_schedule_after(work2.get(), 2000);

    fake_work::check();
    work_run_for(5000);
    fake_work::check(work1, work2, work3);

    CHECK_EQUAL(work1.last_execution(), test_start + 1000);
    CHECK_EQUAL(work2.last_execution(), test_start + 2000);
    CHECK_EQUAL(work3.last_execution(), test_start + 3000);
}

TEST(work, schedule_again)
{
    auto test_start = system_uptime_get_ms();

    fake_work work(0);

    // schedule after 100 ms, but run for 200 ms
    work_schedule_at(work.get(), test_start + 100);
    work_run_for(200);
    CHECK_EQUAL(work.last_execution(), test_start + 100);

    // schedule again after 1000 ms, will execute at 1100 ms
    work_schedule_again(work.get(), 1000);
    work_run_for(5000);
    CHECK_EQUAL(work.last_execution(), test_start + 1100);
}

TEST(work, submit_others)
{
    fake_work work1(1);
    fake_work work3(3);
    fake_work work2(2, [&] {
        work_submit(work3.get());
        work_submit(work1.get());
    });

    work_submit(work2.get());

    work_run_for(0);
    fake_work::check(work2, work1, work3);
}

TEST(work, schedule_self)
{
    fake_work work(0, [&] {
        work_schedule_again(work.get(), 100);
    });

    work_schedule_after(work.get(), 100);
    work_run_for(500);
    fake_work::check(work, work, work, work, work);
}

TEST(work, cancel_scheduled)
{
    auto test_start = system_uptime_get_ms();

    fake_work work2(0);
    fake_work work3(0);
    fake_work work1(0, [&] {
        work_cancel(work2.get());
    });

    work_schedule_after(work3.get(), 3000);
    work_schedule_after(work1.get(), 1000);
    work_schedule_after(work2.get(), 2000);

    fake_work::check();
    work_run_for(5000);
    fake_work::check(work1, work3);

    CHECK_EQUAL(work1.last_execution(), test_start + 1000);
    CHECK_EQUAL(work3.last_execution(), test_start + 3000);
}

TEST(work, cancel_submitted)
{
    fake_work work2(2);
    fake_work work3(3);
    fake_work work1(1, [&] {
        work_cancel(work2.get());
    });

    work_submit(work1.get());
    work_submit(work2.get());
    work_submit(work3.get());

    work_run_for(0);
    fake_work::check(work1, work3);
}

TEST(work, submit_while_scheduled)
{
    auto test_start = system_uptime_get_ms();

    fake_work work(0);

    work_schedule_after(work.get(), 500);
    work_submit(work.get());

    work_run_for(1000);
    fake_work::check(work);  // executed only once
    CHECK_EQUAL(test_start, work.last_execution());  // submitted immediately
}

TEST(work, submit_while_submitted)
{
    fake_work work1(0);
    fake_work work2(0);
    fake_work work3(0);

    work_submit(work1.get());
    work_submit(work2.get());
    work_submit(work3.get());

    work_submit(work2.get());  // submit again

    work_run_for(0);

    fake_work::check(work1, work2, work3);  // executed only once, order does not change
}

TEST(work, schedule_while_scheduled)
{
    auto test_start = system_uptime_get_ms();

    fake_work work(0);

    work_schedule_at(work.get(), test_start + 500);
    work_schedule_at(work.get(), test_start + 100);  // schedule again

    work_run_for(1000);
    fake_work::check(work);  // executed only once
    CHECK_EQUAL(test_start + 500, work.last_execution());  // first schedule wins, even if second is sooner
}
