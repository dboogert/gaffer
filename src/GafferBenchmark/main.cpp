#include <benchmark/benchmark.h>

#include "Gaffer/Context.h"
#include "GafferScene/ScenePlug.h"

static void BM_ContextBorrowedCopy(benchmark::State& state)
{
	for (auto _ : state)
	{
		const Gaffer::Context* current = Gaffer::Context::current();
		Gaffer::Context::Ptr ptr = new Gaffer::Context( *current, Gaffer::Context::Borrowed );
	}
}

BENCHMARK(BM_ContextBorrowedCopy);


static void BM_ContextMakeEditableScope(benchmark::State& state)
{
	for (auto _ : state)
	{
		const Gaffer::Context* current = Gaffer::Context::current();
		Gaffer::Context::EditableScope e(current);
	}
}

BENCHMARK(BM_ContextMakeEditableScope);


static void BM_ContextEditScopeSetFrame(benchmark::State& state)
{
	for (auto _ : state)
	{
		const Gaffer::Context* current = Gaffer::Context::current();
		Gaffer::Context::EditableScope e(current);
		e.setFrame(1.0);
	}
}

BENCHMARK(BM_ContextEditScopeSetFrame);


static void BM_ContextEditScopeSetFrameExistingElement(benchmark::State& state)
{
	const Gaffer::Context* current = Gaffer::Context::current();
	Gaffer::Context::EditableScope e(current);


	for (auto _ : state)
	{
		const Gaffer::Context* current = Gaffer::Context::current();
		Gaffer::Context::EditableScope e2(current);
		e.setFrame(1.0);
	}
}

BENCHMARK(BM_ContextEditScopeSetFrameExistingElement);


static void BM_ContextEditScopeSetPath(benchmark::State& state)
{
	IECore::InternedString path("scene:path");
	GafferScene::ScenePlug::ScenePath p;
	for (auto _ : state)
	{
		const Gaffer::Context* current = Gaffer::Context::current();
		Gaffer::Context::EditableScope e(current);
		e.set(path, p);
	}
}

BENCHMARK(BM_ContextEditScopeSetPath);


static void BM_ContextEditScopeSetRemovePath(benchmark::State& state)
{
	IECore::InternedString path("scene:path");
	GafferScene::ScenePlug::ScenePath p;
	for (auto _ : state)
	{
		const Gaffer::Context* current = Gaffer::Context::current();
		Gaffer::Context::EditableScope e(current);
		e.set(path, p);
		e.remove( path );
	}
}

BENCHMARK(BM_ContextEditScopeSetRemovePath);

BENCHMARK_MAIN();