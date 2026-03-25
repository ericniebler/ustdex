#pragma once

#include "thread_context.hpp"
#include <vector>
#include <random>

#  include "prologue.hpp"

namespace ustdex
{
	struct USTDEX_TYPE_VISIBILITY_DEFAULT static_thread_pool
	{
		class _scheduler
		{
		public:
			USTDEX_API explicit _scheduler(static_thread_pool* _thread_pool) noexcept
				: _thread_pool_{ _thread_pool }
			{}

			[[nodiscard]] USTDEX_API auto schedule() const noexcept
			{
				int i = _thread_pool_->get_rand_index();
				return _thread_pool_->_loops_[i]->get_scheduler().schedule();
			}
		private:
			static_thread_pool* _thread_pool_;
		};

		static_thread_pool(int num = std::thread::hardware_concurrency()*2) noexcept
			: _ss_{ 0, num - 1 }
		{
			_loops_.resize(num);
			_thrds_.resize(num);

			for (int i = 0; i < num; i++)
			{
				_loops_[i] = std::make_unique<run_loop>();
				_thrds_[i] = std::thread{ [this, i]()
					{
						_loops_[i]->run();
					} };
			}
		}

		~static_thread_pool() noexcept
		{
			finish();
			join();
		}

		void finish() noexcept
		{
			for (int i = 0; i < _loops_.size(); i++)
			{
				_loops_[i]->finish();
			}
		}

		void join() noexcept
		{
			for (int i = 0; i < _thrds_.size(); i++)
			{
				if (_thrds_[i].joinable())
				{
					_thrds_[i].join();
				}
			}
		}

		int get_rand_index() const noexcept
		{
			return _ss_(_mt_);
		}

		_scheduler get_scheduler()
		{
			return _scheduler{ this };
		}

	private:
		std::vector<std::thread> _thrds_;
		std::vector<std::unique_ptr<run_loop>> _loops_;

		mutable std::mt19937 _mt_{ std::random_device{}() };
		mutable std::uniform_int_distribution<int> _ss_;
	};
} // namespace ustdex

#  include "epilogue.hpp"