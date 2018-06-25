#include <benchmark/benchmark.h>

#include "IECore/VectorTypedData.h"

#include "Gaffer/Context.h"
#include "GafferScene/ScenePlug.h"

static void BM_ContextBorrowedCopyStack(benchmark::State& state)
{
	const Gaffer::Context c;
	for (auto _ : state)
	{

		Gaffer::Context stackContext ( c, Gaffer::Context::Borrowed );
	}
}

BENCHMARK(BM_ContextBorrowedCopyStack);

static void BM_ContextBorrowedCopy(benchmark::State& state)
{
	const Gaffer::Context* current = Gaffer::Context::current();
	for (auto _ : state)
	{
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
	for (size_t i = 0; i < 16; ++i)
	{
		p.push_back(IECore::InternedString(i));
	}

	for (auto _ : state)
	{
		const Gaffer::Context* current = Gaffer::Context::current();
		Gaffer::Context::EditableScope e(current);
		e.set(path, p);
	}
}

BENCHMARK(BM_ContextEditScopeSetPath);

static void BM_ContextEditScopeSetPathData(benchmark::State& state)
{
	IECore::InternedString path("scene:path");
	IECore::InternedStringVectorDataPtr p = new IECore::InternedStringVectorData();
	for (size_t i = 0; i < 16; ++i)
	{
		p->writable().push_back(IECore::InternedString(i));
	}

	for (auto _ : state)
	{
		const Gaffer::Context* current = Gaffer::Context::current();
		Gaffer::Context::EditableScope e(current);
		e.set(path, p.get());
	}
}

BENCHMARK(BM_ContextEditScopeSetPathData);


static void BM_ContextEditScopeSetRemovePath(benchmark::State& state)
{
	IECore::InternedString path("scene:path");
	GafferScene::ScenePlug::ScenePath p;
	for (size_t i = 0; i < 16; ++i)
	{
		p.push_back(IECore::InternedString(i));
	}

	for (auto _ : state)
	{
		const Gaffer::Context* current = Gaffer::Context::current();
		Gaffer::Context::EditableScope e(current);
		e.set(path, p);
		e.remove( path );
	}
}

BENCHMARK(BM_ContextEditScopeSetRemovePath);
