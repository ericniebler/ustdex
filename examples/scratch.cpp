/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License Version 2.0 with LLVM Exceptions
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *   https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdio>
#include <iostream>

#include "ustdex/ustdex.hpp"
#include <variant>
#include <any>
#include <random>
#include <chrono>
 #include "parallel_sort.hpp"
 //#include "asio_thread_pool.hpp"
#include "ustdex/detail/basic_sender.hpp"


struct sink
{
	using receiver_concept = ustdex::receiver_t;

	void set_value() noexcept {}

	void set_value(int a) noexcept
	{
		std::printf("%d\n", a);
	}

	template <class... As>
	void set_value(As&&...) noexcept
	{
		std::puts("In sink::set_value(auto&&...)");
	}

	void set_error(std::exception_ptr) noexcept {}

	void set_stopped() noexcept {}

	[[nodiscard]]
	ustdex::prop<ustdex::get_stop_token_t, ustdex::inplace_stop_token> get_env() const noexcept
	{
		static ustdex::inplace_stop_source _stop_source_;
		return ustdex::prop{ ustdex::get_stop_token, _stop_source_.get_token() };
	}
};

template <class>
[[deprecated]] void print()
{}

using namespace ustdex;

int main()
{
  ustdex::thread_context tp;
  using MT = decltype(tp.get_scheduler());
  auto task = schedule(tp.get_scheduler()) | let_value([] {
      /*return read_env(get_scheduler) | then([](auto&& sched) {
                         static_assert(std::is_same_v < std::decay_t<decltype(sched)>, std::decay_t<MT>>);
             });*/

	  auto sched_sender = read_env(get_scheduler);
      auto value_sender = just(42);
      return when_all(std::move(sched_sender), std::move(value_sender)) // <-- use a when_all here
          | then([](auto&& sched, int value) { /*io_uring_socket.async_send(value, on=sched)...*/ });
    });

#if 0
	std::cout << "main: " << std::this_thread::get_id() << std::endl;


	ustdex::static_thread_pool thread_pool_b{};
	auto v1 = test_sender(thread_pool_b.get_scheduler(), 2, 3, 4);

	auto op = ustdex::connect(v1, sink{});

	ustdex::start(op);

	while (true);

#endif
	/*auto pp1 = ustdex::_make_sexpr<int>(3, 4, 5);
	_whatis<decltype(pp1)::__captures_t>();*/

	int a = 0;

#if 0
	//asio::static_thread_pool thread_pool_a{};
	ustdex::static_thread_pool thread_pool_b{};
	std::vector<int> values(100'000'000);//
	std::random_device random_device;
	std::mt19937 rng(random_device());
	std::uniform_int_distribution<int> dist(1, 1'000'000);
	std::generate(values.begin(), values.end(), [&] { return dist(rng); });

	std::cout << "starting sort\n";

	auto begin = std::chrono::high_resolution_clock::now();
	//thread_pool_b.get_scheduler()
	/*auto ps = parallel_sort(atp::asio_scheduler{thread_pool_a.get_executor()}, values.begin(), values.end(), [](const int& a, const int& b)
		{
			return a < b;
		});*/

	auto ps = parallel_sort(thread_pool_b.get_scheduler(), values.begin(), values.end(), [](const int& a, const int& b)
		{
			return a < b;
		});
	auto op = ustdex::connect(ps, sink{});
	ustdex::sync_wait(std::move(ps));


	auto end = std::chrono::high_resolution_clock::now();

	auto duration = end - begin;
	std::cout << "sort took "
		<< std::chrono::duration_cast<std::chrono::microseconds>(duration).count()
		<< " microseconds\n";

	return 0;

#endif

#if 0
	auto task = read_env(get_stop_token)
		| then([](auto stop_token)
			{
				std::cout << "Stop token: " << stop_token.stop_requested() << '\n';
				return 42;
			})
		| then([](int i)
			{
				std::cout << "Value: " << i << '\n';
				return i + 1;
			});

	auto task2 = when_all(task);

	//using TT = completion_signatures_of_t<decltype(task)>;

	//_whatis<TT>();

	auto op = connect(task2, sink{});

	start(op);

	//auto [sch2] = sync_wait(read_env(get_scheduler)).value();
#endif

#if 0
	thread_context ctx;
	auto sch = ctx.get_scheduler();

	auto work = just(1, 2, 3) //
		| then([](int a, int b, int c)
			{
				std::printf("%d %d %d\n", a, b, c);
				return a + b + c;
			});
	auto s = starts_on(sch, std::move(work));
	static_assert(!dependent_sender<decltype(s)>);
	std::puts("Hello, world!");
	sync_wait(s);

	auto s3 = just(42) | let_value([](int a)
		{
			std::puts("here");
			return just(a + 1);
		});
	sync_wait(s3);

	auto [sch2] = sync_wait(read_env(get_scheduler)).value();

	auto [i1, i2] = sync_wait(when_all(just(42), just(43))).value();
	std::cout << i1 << ' ' << i2 << '\n';

	auto s4 = just(42) | then([](int) {}) | upon_error([](auto) { /*return 42;*/ });
	auto s5 = when_all(std::move(s4), just(42, 43), just(+"hello"));
	auto [i, j, k] = sync_wait(std::move(s5)).value();
	std::cout << i << ' ' << j << ' ' << k << '\n';

	auto s6 = sequence(just(42) | then([](int)
		{
			std::cout << "sequence sender 1\n";
		}),
		just(42) | then([](int)
			{
				std::cout << "sequence sender 2\n";
			}));
	sync_wait(std::move(s6));

	auto s7 =
		just(42)
		| conditional(
			[](int i)
			{
				return i % 2 == 0;
			},
			then([](int)
				{
					std::cout << "even\n";
				}),
			then([](int)
				{
					std::cout << "odd\n";
				}));
	sync_wait(std::move(s7));


	{
		inplace_stop_source stop_source;

		auto task = just(100)
			| then([&stop_source](int i)
				{
					stop_source.request_stop();
					std::cout << "111_Value: " << i << '\n';
					return i;
				})
			//| stop_when(stop_source.get_token())
			| stop_with([](const int& i)
				{
					if (i == 100)
					{
						return false;
					}
					else
					{
						return false;
					}
				})
			| then([](int i)
				{
					std::cout << "222_Value: " << i << '\n';
					return i;
				})
			| upon_stopped([]()
				{
					std::cout << "Stopped!\n";
				})
			| upon_error([](std::exception_ptr e)
				{
					try
					{
						std::rethrow_exception(e);
					}
					catch (const std::exception& ex)
					{
						std::cout << "Error: " << ex.what() << '\n';
					}
				});

		auto op = connect(std::move(task), sink{});
		start(op);
		//sync_wait(std::move(task));

	}

	{
		auto task = when_any(just(5), just('a'), just(3.14)) |
			then([](std::any v)
				{
					int aa = 0;
				});

		auto op = connect(std::move(task), sink{});
		start(op);

		//using TA = completion_signatures_of_t<decltype(task)>;
		//_m_self_or<_nil>::call
		//using TB = _value_types<TA, _tupl, _variant>;

		//_whatis<TB>();
	}


#endif
}
