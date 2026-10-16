
#include <Windows.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include"macro.h"

namespace fs = std::filesystem;

// 暂时关闭预览模式；true 表示只显示操作，不修改文件
constexpr bool DRY_RUN = false;

// 相对于目标目录 B 的保留规则
const std::vector<std::wstring> KEEP = {
	L".git",
	L"Dependencies",
	L"scripts",
	L"./.gitignore",
	L"./*.q",
	L"./*.md",
	L"./*.py",
	L"./*.jpg",
	L"./*.png",
};

// 转为小写
std::wstring lower(std::wstring s)
{
	std::transform(s.begin(), s.end(), s.begin(),
		[](wchar_t c) { return towlower(c); });
	return s;
}

// 简单通配符匹配：支持 * 和 ?
bool wildcard_match(const std::wstring& pattern,
	const std::wstring& text)
{
	const std::wstring p = lower(pattern);
	const std::wstring s = lower(text);

	size_t i = 0, j = 0;
	size_t star = std::wstring::npos;
	size_t match = 0;

	while (j < s.size())
	{
		if (i < p.size() && (p[i] == L'?' || p[i] == s[j]))
		{
			++i;
			++j;
		}
		else if (i < p.size() && p[i] == L'*')
		{
			star = i++;
			match = j;
		}
		else if (star != std::wstring::npos)
		{
			i = star + 1;
			j = ++match;
		}
		else
		{
			return false;
		}
	}

	while (i < p.size() && p[i] == L'*')
		++i;

	return i == p.size();
}

// 判断相对路径是否属于保留范围
bool is_kept(const fs::path& relative_path)
{
	const std::wstring rel =
		relative_path.generic_wstring();

	for (const auto& item : KEEP)
	{
		std::wstring pattern = item;

		// 根目录规则，例如 .git、Dependencies
		if (pattern.rfind(L"./", 0) == 0)
			pattern.erase(0, 2);

		const bool has_wildcard =
			pattern.find_first_of(L"*?[") != std::wstring::npos;

		if (pattern.find(L'/') == std::wstring::npos &&
			!has_wildcard)
		{
			if (lower(rel) == lower(pattern) ||
				lower(rel).rfind(lower(pattern) + L"/", 0) == 0)
			{
				return true;
			}
		}

		// 仅匹配项目根目录文件，例如 ./*.png
		if (item.rfind(L"./", 0) == 0)
		{
			const std::wstring root_pattern = item.substr(2);

			if (rel.find(L'/') == std::wstring::npos &&
				wildcard_match(root_pattern, rel))
			{
				return true;
			}

			continue;
		}

		// 其他通配符匹配
		if (wildcard_match(pattern, rel))
			return true;
	}

	return false;
}

// 比较两个文件的实际内容
bool same_file(const fs::path& a, const fs::path& b)
{
	std::error_code ec;

	const auto size_a = fs::file_size(a, ec);
	if (ec)
		return false;

	const auto size_b = fs::file_size(b, ec);
	if (ec || size_a != size_b)
		return false;

	std::ifstream fa(a, std::ios::binary);
	std::ifstream fb(b, std::ios::binary);

	if (!fa || !fb)
		return false;

	char ba[64 * 1024];
	char bb[64 * 1024];

	while (fa && fb)
	{
		fa.read(ba, sizeof(ba));
		fb.read(bb, sizeof(bb));

		const auto na = fa.gcount();
		const auto nb = fb.gcount();

		if (na != nb ||
			!std::equal(ba, ba + na, bb))
		{
			return false;
		}
	}

	return fa.eof() && fb.eof();
}

// 判断是否为符号链接
bool is_symbol_link(const fs::path& p)
{
	std::error_code ec;
	return fs::is_symlink(fs::symlink_status(p, ec));
}

// 复制目录属性
void copy_directory_attributes(
	const fs::path& src,
	const fs::path& dst)
{
	DWORD attrs = GetFileAttributesW(src.c_str());

	if (attrs != INVALID_FILE_ATTRIBUTES)
		SetFileAttributesW(dst.c_str(), attrs);
}

// 自动查找源目录 A
fs::path find_source(const std::wstring& project_name)
{
	const std::vector<fs::path> candidates = {
		fs::path(LR"(C:\Users\Pudge\source\repos)") / project_name,
		fs::path(LR"(C:\Users\Pudge\source\repos\Misc)") / project_name,
		fs::path(LR"(D:\Work)") / project_name,
		fs::path(LR"(D:\Work\Misc)") / project_name,
	};

	for (const auto& path : candidates)
	{
		std::error_code ec;

		if (fs::is_directory(path, ec))
			return path;
	}

	return {};
}

// 同步
void sync()
{
	// B 是执行命令时所在的目录
	const fs::path B = fs::current_path();
	const std::wstring project_name = B.filename().wstring();

	// A 根据项目名称自动查找
	const fs::path A = find_source(project_name);

	if (A.empty())
	{
		throw std::runtime_error(
			"Cannot find source project directory A.");
	}

	std::error_code ec;

	const fs::path canonicalA = fs::canonical(A, ec);
	if (ec)
		throw std::runtime_error("Cannot resolve source directory A.");

	const fs::path canonicalB = fs::canonical(B, ec);
	if (ec)
		throw std::runtime_error("Cannot resolve destination directory B.");

	if (canonicalA == canonicalB)
	{
		A_START;
		throw std::runtime_error(
			"❌拒绝执行：当前目录是源目录 A，请切换到目标目录 B。");//not really wide character string
		A_STOP;

	}

	// 避免意外地把同一个目录作为源和目标
	if (fs::equivalent(A, B))
		throw std::runtime_error(
			"Source and destination are the same directory.");
	A_START;
	printf("✅ 检查通过，开始执行核心任务...\n");
	A_STOP;

	std::wcout << L"项目名称： " << project_name << L'\n';
	std::wcout << L"源目录 A： " << A.wstring() << L'\n';
	std::wcout << L"目标目录 B： " << B.wstring() << L'\n';

	// 第一阶段：复制和更新文件
	for (const auto& entry :
		fs::recursive_directory_iterator(A))
	{
		const fs::path src = entry.path();
		const fs::path rel = src.lexically_relative(A);
		const fs::path dst = B / rel;

		if (is_symlink(src))
		{
			std::wcout << L"跳过符号链接："
				<< rel.wstring() << L'\n';
			continue;
		}

		if (entry.is_directory())
		{
			std::error_code ec;

			if (!fs::exists(dst, ec) && !DRY_RUN)
			{
				fs::create_directories(dst);
				copy_directory_attributes(src, dst);
			}

			continue;
		}

		if (!entry.is_regular_file())
			continue;

		// 已存在的保留文件不覆盖
		std::error_code ec;
		if (is_kept(rel) && fs::exists(dst, ec))
			continue;

		const bool need_copy =
			!fs::exists(dst, ec) ||
			!fs::is_regular_file(dst, ec) ||
			!same_file(src, dst);

		if (need_copy)
		{
			std::wcout << L"复制/更新："
				<< rel.wstring() << L'\n';

			if (!DRY_RUN)
			{
				fs::create_directories(dst.parent_path());

				fs::copy_file(
					src, dst,
					fs::copy_options::overwrite_existing);

				// 尽量保留源文件的修改时间
				fs::last_write_time(
					dst, fs::last_write_time(src));
			}
		}
	}

	// 第二阶段：删除目标目录中源目录不存在的文件
	std::vector<fs::path> entries;

	for (const auto& entry :
		fs::recursive_directory_iterator(B))
	{
		entries.push_back(entry.path());
	}

	// 深层路径优先处理
	std::sort(entries.begin(), entries.end(),
		[](const fs::path& a, const fs::path& b)
		{
			return std::distance(a.begin(), a.end()) >
				std::distance(b.begin(), b.end());
		});

	for (const auto& dst : entries)
	{
		if (is_symbol_link(dst))
			continue;

		const fs::path rel = dst.lexically_relative(B);

		if (is_kept(rel))
			continue;

		const fs::path src = A / rel;

		std::error_code ec;

		if (fs::is_regular_file(dst, ec) &&
			!fs::is_regular_file(src, ec))
		{
			std::wcout << L"删除文件："
				<< rel.wstring() << L'\n';

			if (!DRY_RUN)
				fs::remove(dst);
		}
		else if (fs::is_directory(dst, ec) &&
			!fs::is_directory(src, ec))
		{
			if (!fs::is_empty(dst, ec))
				continue;

			std::wcout << L"删除空目录："
				<< rel.wstring() << L'\n';

			if (!DRY_RUN)
				fs::remove(dst);
		}
	}

	std::wcout << (DRY_RUN
		? L"\n预览完成。\n"
		: L"\n同步完成。\n");
}

