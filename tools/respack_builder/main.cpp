
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>

#include <respack.h>

#include "collectors/image.h"
#include "collectors/json.h"
#include "collectors/shader.h"
#include "interfaces/writer.h"

using namespace respack;

struct FilesProcess {
  Collector *collector;
  std::vector<std::pair<std::string, std::filesystem::path>> files;
};

static std::unordered_map<std::string, FilesProcess> processes;

static std::uint32_t search_assets(std::string prefix, const std::filesystem::path &p) {
  std::uint32_t count = 0;
  for (auto &ent : std::filesystem::directory_iterator(p)) {
    auto &path = ent.path();
    auto name = prefix + path.filename().string();
    if (ent.is_directory()) {
      count += search_assets(std::move(name) + "/", path);
    } else if (ent.is_regular_file()) {
      auto ext = path.extension();
      auto it = processes.find(ext.string());
      if (it == processes.end()) {
        continue;
      }
      it->second.files.emplace_back(std::move(name), path);
      ++count;
    }
  }
  return count;
}

template <class Tp>
void WriteStruct(Writer &writer, const Tp &val) {
  writer.Write(&val, sizeof(Tp));
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: respack_builder [assets directory] [respack output location]\n";
    return 1;
  }

  std::cout << "Begin to build .respack...\n";

  auto begin_time = std::chrono::steady_clock::now();

  auto output = std::filesystem::path(argv[2]);
  std::ofstream out(output, std::ios::binary);
  if (!out.is_open()) {
    std::cerr << "Can't not write data to " << output.lexically_normal() << "!\n";
    return 1;
  }

  Writer writer(out);

  ShaderCollector shader_collector;
  ImageCollector image_collector;
  JsonCollector json_collector;

  std::uint32_t resources_count;
  {
    std::filesystem::path assets = argv[1];
    if (!std::filesystem::is_directory(assets)) {
      std::cerr << "Could not open assets folder: " << assets.lexically_normal() << '\n';
      return 1;
    }

    processes[".png"] = {&image_collector};
    processes[".spv"] = {&shader_collector};
    processes[".json"] = {&json_collector};

    resources_count = search_assets("", std::move(assets));
  }

  auto detected_time = std::chrono::steady_clock::now();
  {
    auto spent = std::chrono::duration_cast<std::chrono::milliseconds>(detected_time - begin_time);
    std::cout << "All resources was detected. Count: " << resources_count << ". (" << spent.count() << "ms)\n";
    std::cout << "Output path: " << output.lexically_normal() << '\n';
  }

  FileHeader header{kHeaderMagic, 2};
  WriteStruct(writer, header);

  auto content_size = sizeof(ResourceDescriptor) * resources_count;

  ResourceTableSectionHeader resources_header;
  resources_header.count = resources_count;
  resources_header.type = SectionType::kResourceTable;
  resources_header.length = std::uint32_t(sizeof(ResourceTableSectionHeader) + content_size);
  WriteStruct(writer, resources_header);

  std::vector<ResourceDescriptor> desc;

  auto start = writer.Length() + content_size;
  for (auto &[_, p] : processes) {
    std::cout << "These files are treated as " << p.collector->GetTypeName() << ":\n";
    for (auto &[name, path] : p.files) {
      auto res = p.collector->CreateResourceDescriptor(path);
      res.name_length = name.size();
      res.name_offset = start;

      start += res.name_length;
      res.data_offset = start;
      start += res.data_length;

      desc.emplace_back(res);
      WriteStruct(writer, res);

      auto file_size = std::filesystem::file_size(path);
      auto ratio = double(res.data_length) / file_size * 100;

      std::cout << '\t' << name << " ("
                << file_size << " Bytes -> "
                << res.data_length << " Bytes, "
                << ratio << "%)\n";
    }
    std::cout << '\n';
  }

  auto it = desc.begin();
  for (auto &[_, p] : processes) {
    for (auto &[name, _] : p.files) {
      out.seekp(it->name_offset);
      out << name;
      ++it;
    }
  }

  it = desc.begin();
  for (auto &[_, p] : processes) {
    for (auto &[name, path] : p.files) {
      out.seekp(it->data_offset);
      p.collector->WriteData(path, writer);
      ++it;
    }
  }

  auto end_time = std::chrono::steady_clock::now();
  {
    auto spent = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - detected_time);
    std::cout << "default.respack (" << writer.Length() / 1024 << " KB) was built. (" << spent.count() << "ms)\n";
  }

  return 0;
}
