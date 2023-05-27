#pragma once
#ifndef DISPOSABLE_OBJECT
#define DISPOSABLE_OBJECT
	#include "invoke_callback.hpp"
	
	#ifndef DISPOSABLE_BOOL_DEFAULT
		#define DISPOSABLE_BOOL_DEFAULT true
	#endif

	class disposable {
	protected:
		std::atomic_bool disposed = false;

	public:

		invokable<bool> onDispose;
			
		void Dispose() {
			if (disposed) return;
			onDispose.invoke(DISPOSABLE_BOOL_DEFAULT);
			disposed = true;
		}

		bool IsDisposed() { return disposed; }

		~disposable() { Dispose(); }

		static void DisposeOrdered(std::vector<disposable*> objects, bool reverseOrder = false) {
			if (reverseOrder) std::reverse(objects.begin(), objects.end());

			for(disposable* obj : objects) obj->Dispose();
		}
	};

#endif