// This file is part of the QuantumGate project. For copyright and
// licensing information refer to the license file(s) in the project root.

#pragma once

#include "KeyGenerationEvent.h"
#include "..\..\Concurrency\ThreadPool.h"
#include "..\..\Concurrency\Queue.h"

namespace QuantumGate::Implementation::Core::KeyGeneration
{
	class Manager final
	{
		using KeyQueueMap = Containers::UnorderedMap<Algorithm::Asymmetric, std::unique_ptr<KeyQueue_ThS>>;
		using KeyQueueMap_ThS = Concurrency::ThreadSafe<KeyQueueMap, Concurrency::SharedSpinMutex>;

		using EventQueue = Concurrency::Queue<Event>;
		using EventQueue_ThS = Concurrency::ThreadSafe<EventQueue, Concurrency::SpinMutex>;

		struct ThreadPoolData final
		{
			EventQueue_ThS KeyGenEventQueue;
			Concurrency::Event PrimaryThreadEvent;
		};

		using ThreadPool = Concurrency::ThreadPool<ThreadPoolData>;

	public:
		Manager(const Settings_CThS& settings) noexcept;
		Manager(const Manager&) = delete;
		Manager(Manager&&) noexcept = default;
		~Manager() { if (IsRunning()) Shutdown(); }
		Manager& operator=(const Manager&) = delete;
		Manager& operator=(Manager&&) noexcept = default;

		const Settings& GetSettings() const noexcept;

		bool Startup() noexcept;
		void Shutdown() noexcept;

		inline bool IsRunning() const noexcept { return m_Running; }

		std::optional<Crypto::AsymmetricKeyData> GetAsymmetricKeys(const Algorithm::Asymmetric alg) noexcept;

	private:
		void PreStartup() noexcept;
		void ResetState() noexcept;

		bool AddKeyQueues() noexcept;
		void ClearKeyQueues() noexcept;

		bool StartupThreadPool() noexcept;
		void ShutdownThreadPool() noexcept;

		ThreadPool::ThreadCallbackResult PrimaryThreadProcessor(ThreadPoolData& thpdata,
																const Concurrency::Event& shutdown_event);

		ThreadPool::ThreadCallbackResult WorkerThreadProcessor(ThreadPoolData& thpdata,
															   const Concurrency::Event& shutdown_event);

	private:
		std::atomic_bool m_Running{ false };
		const Settings_CThS& m_Settings;

		KeyQueueMap_ThS m_KeyQueues;
		ThreadPool m_ThreadPool;
	};
}