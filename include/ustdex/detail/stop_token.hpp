/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef USTDEX_ASYNC_DETAIL_STOP_TOKEN
#define USTDEX_ASYNC_DETAIL_STOP_TOKEN

#include "config.hpp"
#include "thread.hpp"
#include "utility.hpp"

#include <atomic>
#if __has_include(<stop_token>) && __cpp_lib_jthread >= 201911
#  include <stop_token>
#endif

#include "prologue.hpp"

// warning #20012-D: __device__ annotation is ignored on a
// function("inplace_stop_source") that is explicitly defaulted on its first
// declaration
USTDEX_PRAGMA_PUSH()
USTDEX_PRAGMA_IGNORE_EDG(20012)

#if USTDEX_ARCH(ARM64) && defined(_linux__)
#  define USTDEX_ASM_THREAD_YIELD (asm volatile("yield" :: :);)
#elif USTDEX_ARCH(X86_64) && defined(_linux__)
#  define USTDEX_ASM_THREAD_YIELD (asm volatile("pause" :: :);)
#else // ^^^  USTDEX_ARCH(X86_64) ^^^ / vvv ! USTDEX_ARCH(X86_64) vvv
#  define USTDEX_ASM_THREAD_YIELD (;)
#endif // ! USTDEX_ARCH(X86_64)

USTDEX_API void _ustdex_thread_yield_processor()
{
  USTDEX_IF_TARGET(USTDEX_IS_HOST, USTDEX_ASM_THREAD_YIELD)
}

namespace ustdex
{
// [stoptoken.inplace], class inplace_stop_token
class inplace_stop_token;

// [stopsource.inplace], class inplace_stop_source
class inplace_stop_source;

// [stopcallback.inplace], class template inplace_stop_callback
template <class Callback>
class inplace_stop_callback;

namespace _stok
{
struct _inplace_stop_callback_base
{
  USTDEX_API void _execute() noexcept
  {
    this->_execute_fn_(this);
  }

protected:
  using _execute_fn_t = void(_inplace_stop_callback_base*) noexcept;

  USTDEX_API explicit _inplace_stop_callback_base( //
    const inplace_stop_source* _source, //
    _execute_fn_t* _execute) noexcept
      : _source_(_source)
      , _execute_fn_(_execute)
  {}

  USTDEX_API void _register_callback() noexcept;

  friend inplace_stop_source;

  const inplace_stop_source* _source_;
  _execute_fn_t* _execute_fn_;
  _inplace_stop_callback_base* _next_      = nullptr;
  _inplace_stop_callback_base** _prev_ptr_ = nullptr;
  bool* _removed_during_callback_          = nullptr;
  std::atomic<bool> _callback_completed_{false};
};

struct _spin_wait
{
  _spin_wait() noexcept = default;

  USTDEX_API void _wait() noexcept
  {
    if (_count_ == 0)
    {
      ustdex::_this_thread_yield();
    }
    else
    {
      --_count_;
      _ustdex_thread_yield_processor();
    }
  }

private:
  static constexpr uint32_t _yield_threshold = 20;
  uint32_t _count_                           = _yield_threshold;
};

template <template <class> class>
struct _check_type_alias_exists;
} // namespace _stok

// [stoptoken.never], class never_stop_token
struct never_stop_token
{
private:
  struct _callback_type
  {
    USTDEX_API explicit _callback_type(never_stop_token, _ignore) noexcept {}
  };

public:
  template <class>
  using callback_type = _callback_type;

  USTDEX_API static constexpr auto stop_requested() noexcept -> bool
  {
    return false;
  }

  USTDEX_API static constexpr auto stop_possible() noexcept -> bool
  {
    return false;
  }

  USTDEX_API friend constexpr bool operator==(const never_stop_token&, const never_stop_token&) noexcept
  {
    return true;
  }

  USTDEX_API friend constexpr bool operator!=(const never_stop_token&, const never_stop_token&) noexcept
  {
    return false;
  }
};

template <class Callback>
class inplace_stop_callback;

// [stopsource.inplace], class inplace_stop_source
class inplace_stop_source
{
public:
  USTDEX_API inplace_stop_source() noexcept = default;
  USTDEX_API ~inplace_stop_source();
  USTDEX_IMMOVABLE(inplace_stop_source);

  USTDEX_API auto get_token() const noexcept -> inplace_stop_token;

  USTDEX_API auto request_stop() noexcept -> bool;

  USTDEX_API auto stop_requested() const noexcept -> bool
  {
    return (_state_.load(std::memory_order_acquire) & _stop_requested_flag) != 0;
  }

private:
  friend inplace_stop_token;
  friend _stok::_inplace_stop_callback_base;
  template <class>
  friend class inplace_stop_callback;

  USTDEX_API auto _lock() const noexcept -> uint8_t;
  USTDEX_API void _unlock(uint8_t) const noexcept;

  USTDEX_API auto _try_lock_unless_stop_requested(bool) const noexcept -> bool;

  USTDEX_API auto _try_add_callback(_stok::_inplace_stop_callback_base*) const noexcept -> bool;

  USTDEX_API void _remove_callback(_stok::_inplace_stop_callback_base*) const noexcept;

  static constexpr uint8_t _stop_requested_flag = 1;
  static constexpr uint8_t _locked_flag         = 2;

  mutable std::atomic<uint8_t> _state_{0};
  mutable _stok::_inplace_stop_callback_base* _callbacks_ = nullptr;
  ustdex::_thread_id _notifying_thread_;
};

// [stoptoken.inplace], class inplace_stop_token
class inplace_stop_token
{
public:
  template <class Fun>
  using callback_type = inplace_stop_callback<Fun>;

  USTDEX_API inplace_stop_token() noexcept
      : _source_(nullptr)
  {}

  inplace_stop_token(const inplace_stop_token& _other) noexcept = default;

  USTDEX_API inplace_stop_token(inplace_stop_token&& _other) noexcept
      : _source_(ustdex::_exchange(_other._source_, {}))
  {}

  auto operator=(const inplace_stop_token& _other) noexcept -> inplace_stop_token& = default;

  USTDEX_API auto operator=(inplace_stop_token&& _other) noexcept -> inplace_stop_token&
  {
    _source_ = ustdex::_exchange(_other._source_, nullptr);
    return *this;
  }

  [[nodiscard]] USTDEX_API auto stop_requested() const noexcept -> bool
  {
    return _source_ != nullptr && _source_->stop_requested();
  }

  [[nodiscard]] USTDEX_API auto stop_possible() const noexcept -> bool
  {
    return _source_ != nullptr;
  }

  USTDEX_API void swap(inplace_stop_token& _other) noexcept
  {
    ustdex::_swap(_source_, _other._source_);
  }

  USTDEX_API friend bool operator==(const inplace_stop_token& _a, const inplace_stop_token& _b) noexcept
  {
    return _a._source_ == _b._source_;
  }

  USTDEX_API friend bool operator!=(const inplace_stop_token& _a, const inplace_stop_token& _b) noexcept
  {
    return _a._source_ != _b._source_;
  }

private:
  friend inplace_stop_source;
  template <class>
  friend class inplace_stop_callback;

  USTDEX_API explicit inplace_stop_token(const inplace_stop_source* _source) noexcept
      : _source_(_source)
  {}

  const inplace_stop_source* _source_;
};

USTDEX_API inline auto inplace_stop_source::get_token() const noexcept -> inplace_stop_token
{
  return inplace_stop_token{this};
}

// [stopcallback.inplace], class template inplace_stop_callback
template <class Fun>
class inplace_stop_callback : _stok::_inplace_stop_callback_base
{
public:
  template <class Fun2>
  USTDEX_API explicit inplace_stop_callback(inplace_stop_token _token,
                                            Fun2&& _fun) noexcept(std::is_nothrow_constructible_v<Fun, Fun2>)
      : _stok::_inplace_stop_callback_base(_token._source_, &inplace_stop_callback::_execute_impl)
      , _fun(static_cast<Fun2&&>(_fun))
  {
    _register_callback();
  }

  USTDEX_API ~inplace_stop_callback()
  {
    if (_source_ != nullptr)
    {
      _source_->_remove_callback(this);
    }
  }

private:
  USTDEX_API static void _execute_impl(_stok::_inplace_stop_callback_base* _cb) noexcept
  {
    static_cast<Fun&&>(static_cast<inplace_stop_callback*>(_cb)->_fun)();
  }

  USTDEX_NO_UNIQUE_ADDRESS Fun _fun;
};

namespace _stok
{
USTDEX_API inline void _inplace_stop_callback_base::_register_callback() noexcept
{
  if (_source_ != nullptr)
  {
    if (!_source_->_try_add_callback(this))
    {
      _source_ = nullptr;
      // Callback not registered because stop_requested() was true.
      // Execute inline here.
      _execute();
    }
  }
}
} // namespace _stok

USTDEX_API inline inplace_stop_source::~inplace_stop_source()
{
  USTDEX_ASSERT((_state_.load(std::memory_order_relaxed) & _locked_flag) == 0, "");
  USTDEX_ASSERT(_callbacks_ == nullptr, "");
}

USTDEX_API inline auto inplace_stop_source::request_stop() noexcept -> bool
{
  if (!_try_lock_unless_stop_requested(true))
  {
    return true;
  }

  _notifying_thread_ = ustdex::_this_thread_id();

  // We are responsible for executing callbacks.
  while (_callbacks_ != nullptr)
  {
    auto* _callbk       = _callbacks_;
    _callbk->_prev_ptr_ = nullptr;
    _callbacks_         = _callbk->_next_;
    if (_callbacks_ != nullptr)
    {
      _callbacks_->_prev_ptr_ = &_callbacks_;
    }

    _state_.store(_stop_requested_flag, std::memory_order_release);

    bool _removed_during_callback_     = false;
    _callbk->_removed_during_callback_ = &_removed_during_callback_;

    _callbk->_execute();

    if (!_removed_during_callback_)
    {
      _callbk->_removed_during_callback_ = nullptr;
      _callbk->_callback_completed_.store(true, std::memory_order_release);
    }

    _lock();
  }

  _state_.store(_stop_requested_flag, std::memory_order_release);
  return false;
}

USTDEX_API inline auto inplace_stop_source::_lock() const noexcept -> uint8_t
{
  _stok::_spin_wait _spin;
  auto _old_state = _state_.load(std::memory_order_relaxed);
  do
  {
    while ((_old_state & _locked_flag) != 0)
    {
      _spin._wait();
      _old_state = _state_.load(std::memory_order_relaxed);
    }
  } while (!_state_.compare_exchange_weak(
    _old_state, _old_state | _locked_flag, std::memory_order_acquire, std::memory_order_relaxed));

  return _old_state;
}

USTDEX_API inline void inplace_stop_source::_unlock(uint8_t _old_state) const noexcept
{
  (void) _state_.store(_old_state, std::memory_order_release);
}

USTDEX_API inline auto inplace_stop_source::_try_lock_unless_stop_requested(bool _set_stop_requested) const noexcept
  -> bool
{
  _stok::_spin_wait _spin;
  auto _old_state = _state_.load(std::memory_order_relaxed);
  do
  {
    while (true)
    {
      if ((_old_state & _stop_requested_flag) != 0)
      {
        // Stop already requested.
        return false;
      }
      else if (_old_state == 0)
      {
        break;
      }
      else
      {
        _spin._wait();
        _old_state = _state_.load(std::memory_order_relaxed);
      }
    }
  } while (!_state_.compare_exchange_weak(
    _old_state,
    _set_stop_requested ? (_locked_flag | _stop_requested_flag) : _locked_flag,
    std::memory_order_acq_rel,
    std::memory_order_relaxed));

  // Lock acquired successfully
  return true;
}

USTDEX_API inline auto
inplace_stop_source::_try_add_callback(_stok::_inplace_stop_callback_base* _callbk) const noexcept -> bool
{
  if (!_try_lock_unless_stop_requested(false))
  {
    return false;
  }

  _callbk->_next_     = _callbacks_;
  _callbk->_prev_ptr_ = &_callbacks_;
  if (_callbacks_ != nullptr)
  {
    _callbacks_->_prev_ptr_ = &_callbk->_next_;
  }
  _callbacks_ = _callbk;

  _unlock(0);

  return true;
}

USTDEX_API inline void inplace_stop_source::_remove_callback(_stok::_inplace_stop_callback_base* _callbk) const noexcept
{
  auto _old_state = _lock();

  if (_callbk->_prev_ptr_ != nullptr)
  {
    // Callback has not been executed yet.
    // Remove from the list.
    *_callbk->_prev_ptr_ = _callbk->_next_;
    if (_callbk->_next_ != nullptr)
    {
      _callbk->_next_->_prev_ptr_ = _callbk->_prev_ptr_;
    }
    _unlock(_old_state);
  }
  else
  {
    auto _notifying_thread_ = this->_notifying_thread_;
    _unlock(_old_state);

    // Callback has either already been executed or is
    // currently executing on another thread.
    if (ustdex::_this_thread_id() == _notifying_thread_)
    {
      if (_callbk->_removed_during_callback_ != nullptr)
      {
        *_callbk->_removed_during_callback_ = true;
      }
    }
    else
    {
      // Concurrently executing on another thread.
      // Wait until the other thread finishes executing the callback.
      _stok::_spin_wait _spin;
      while (!_callbk->_callback_completed_.load(std::memory_order_acquire))
      {
        _spin._wait();
      }
    }
  }
}

struct _on_stop_request
{
  inplace_stop_source& _source_;

  USTDEX_API void operator()() const noexcept
  {
    _source_.request_stop();
  }
};

template <class Token, class Callback>
using stop_callback_for_t = typename Token::template callback_type<Callback>;
} // namespace ustdex

#include "epilogue.hpp"

#endif
