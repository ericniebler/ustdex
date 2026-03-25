#pragma once

#include <ustdex/ustdex.hpp>
#include <asio.hpp>

namespace atp
{
	template <class Rcvr>
	struct _operation
	{
		asio::any_io_executor _executor_;
		USTDEX_NO_UNIQUE_ADDRESS Rcvr _rcvr_;

		USTDEX_API static void _execute_impl(_operation* _p) noexcept
		{
			auto& _rcvr = _p->_rcvr_;
			try
			{
				if (ustdex::get_stop_token(ustdex::get_env(_rcvr)).stop_requested())
				{
					ustdex::set_stopped(static_cast<Rcvr&&>(_rcvr));
				}
				else
				{
					ustdex::set_value(static_cast<Rcvr&&>(_rcvr));
				}
			}
			catch (...)
			{
				ustdex::set_error(static_cast<Rcvr&&>(_rcvr), ::std::current_exception());
			}

		}

		USTDEX_API _operation(asio::any_io_executor executor, Rcvr _rcvr) :
			_executor_{ executor }
			, _rcvr_{ static_cast<Rcvr&&>(_rcvr) }
		{}

		USTDEX_API void start() & noexcept;
	};

	class asio_scheduler
	{
		struct _schedule_task
		{
			using sender_concept = ustdex::sender_t;

			template <class Rcvr>
			USTDEX_API auto connect(Rcvr _rcvr) const noexcept -> _operation<Rcvr>
			{
				return { _executor_, static_cast<Rcvr&&>(_rcvr) };
			}

			template <class Self>
			USTDEX_API static constexpr auto get_completion_signatures() noexcept
			{
				return ustdex::completion_signatures<ustdex::set_value_t(), ustdex::set_error_t(::std::exception_ptr), ustdex::set_stopped_t()>();
			}

		private:
			friend asio_scheduler;

			struct _env
			{
				asio::any_io_executor _executor_;

				template <class Tag>
				USTDEX_API auto query(ustdex::get_completion_scheduler_t<Tag>) const noexcept -> asio_scheduler
				{
					return asio_scheduler{ _executor_ };
				}
			};

			USTDEX_API auto get_env() const noexcept -> _env
			{
				return _env{ _executor_ };
			}

			USTDEX_API explicit _schedule_task(asio::any_io_executor executor) noexcept
				: _executor_(executor)
			{}

			asio::any_io_executor const _executor_;
		};

		USTDEX_API auto query(ustdex::get_forward_progress_guarantee_t) const noexcept -> ustdex::forward_progress_guarantee
		{
			return ustdex::forward_progress_guarantee::parallel;
		}

		asio::any_io_executor _executor_;

	public:
		using scheduler_concept = ustdex::scheduler_t;

		USTDEX_API explicit asio_scheduler(asio::any_io_executor executor) noexcept
			: _executor_(executor)
		{}

		[[nodiscard]] USTDEX_API auto schedule() const noexcept -> _schedule_task
		{
			return _schedule_task{ _executor_ };
		}

		USTDEX_API friend bool operator==(const asio_scheduler& _a, const asio_scheduler& _b) noexcept
		{
			return _a._executor_ == _b._executor_;
		}

		USTDEX_API friend bool operator!=(const asio_scheduler& _a, const asio_scheduler& _b) noexcept
		{
			return _a._executor_ != _b._executor_;
		}
	};

	template <class Rcvr>
	USTDEX_API inline void _operation<Rcvr>::start() & noexcept
	{
		try
		{
			asio::post(_executor_,
				[this]()
				{
					_execute_impl(this);
				});
		}
		catch(...)
		{
			ustdex::set_error(static_cast<Rcvr&&>(_rcvr_), ::std::current_exception());
		}

	}

}