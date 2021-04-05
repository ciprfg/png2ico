#include <iostream>
#include <fstream>
#include <vector>

constexpr auto PNG_HEADER_SIZE = 8;
constexpr auto PNG_CHUNK_SIZE = 8;

struct png_image
{
	uint32_t width;             /* png image width */
	uint32_t height;            /* png image height */
	std::vector<uint8_t>buffer; /* png file */
};

constexpr auto ICO_RESOURCE_TYPE = 1;

struct icon_header
{
	uint16_t reserved; /* reserved, must be 0 */
	uint16_t type;     /* resource type, 1 for icon */
	uint16_t count;    /* icon entry count */
};

struct icon_entry
{
	uint8_t width;       /* image width */
	uint8_t height;      /* image height */
	uint8_t color_count; /* nr. of colors in image, 0 if >= 8bpp */
	uint8_t reserved;    /* reserved, must be 0 */
	uint16_t planes;     /* color planes */
	uint16_t bit_count;  /* bits per pixel */
	uint32_t size;       /* image size in bytes */
	uint32_t offset;     /* image offset in file */
};

inline uint32_t swap_uint32(uint32_t value)
{
	value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
	return (value << 16) | (value >> 16);
}

inline int check_png_signature(const uint8_t* signature)
{
	const std::vector<uint8_t> png_signature = {
		0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
	return memcmp(&signature[0], &png_signature[0], PNG_HEADER_SIZE);
}

png_image read_png_file(const char* file_name)
{
	png_image image;
	std::ifstream png_file(file_name, std::ios::binary);
	if (!png_file.is_open())
	{
		std::cerr << "error: unable to open file " << file_name << "\n";
		exit(-1);
	}

	uint8_t png_header[PNG_HEADER_SIZE];
	png_file.read(reinterpret_cast<char*>(&png_header), PNG_HEADER_SIZE);
	if (check_png_signature(png_header))
	{
		std::cerr << "error: " << file_name << " is not a png file\n";
		exit(-1);
	}

	png_file.seekg(PNG_CHUNK_SIZE, std::ios::cur);
	png_file.read(reinterpret_cast<char*>(&image.width), sizeof(image.width));
	png_file.read(reinterpret_cast<char*>(&image.height), sizeof(image.height));
	image.width = swap_uint32(image.width);
	image.height = swap_uint32(image.height);

	if (image.width != image.height)
	{
		std::cerr << "error: png image should be square, width = height\n";
		exit(-1);
	}

	if (image.width > 512)
	{
		std::cerr << "error: png width and height should be smaller than 512 pixels\n";
		exit(-1);
	}

	png_file.seekg(std::ios::beg);
	std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(png_file), {});
	image.buffer = buffer;

	return image;
}

void write_ico_file(const char* file_name, std::vector<png_image> png)
{
	std::ofstream ico_file(file_name, std::ios::binary);
	if (!ico_file.is_open())
	{
		std::cerr << "error: unable to open file " << file_name << "\n";
		exit(-1);
	}

	icon_header header;
	header.reserved = 0;
	header.type = ICO_RESOURCE_TYPE;
	header.count = static_cast<uint16_t>(png.size());

	ico_file.write(reinterpret_cast<char*>(&header), sizeof(header));
	if (ico_file.fail())
	{
		std::cerr << "error: unable to write ico header to " << file_name << "\n";
		exit(-1);
	}

	uint32_t offset = (sizeof(icon_header) +
		(sizeof(icon_entry) * header.count));

	for (auto i = 0; i < header.count; i++)
	{
		icon_entry entry;
		entry.width = (png[i].width > 255) ? 0 : png[i].width;
		entry.height = (png[i].height > 255) ? 0 : png[i].height;
		entry.color_count = 0;
		entry.reserved = 0;
		entry.planes = 1;
		entry.bit_count = 32;
		entry.offset = offset;
		entry.size = static_cast<uint32_t>(png[i].buffer.size());

		ico_file.write(reinterpret_cast<char*>(&entry), sizeof(entry));
		if (ico_file.fail())
		{
			std::cerr << "error: unable to write ico entry to " << file_name << "\n";
			exit(-1);
		}

		offset += entry.size;
	}

	for (auto i = 0; i < header.count; i++)
	{
		ico_file.write(reinterpret_cast<char*>(&png[i].buffer[0]),
			png[i].buffer.size());
		if (ico_file.fail())
		{
			std::cerr << "error: unable to write png image to " << file_name << "\n";
			exit(-1);
		}
	}
}

int main(int argc, char const* argv[])
{
	constexpr auto MAX_ICON_COUNT = 16;

	if (argc < 3)
	{
		std::cerr << "error: no input files specified\n";
		return -1;
	}

	std::vector<png_image> png;
	auto count = ((argc - 2) > MAX_ICON_COUNT) ? MAX_ICON_COUNT : (argc - 2);

	for (auto i = 0; i < count; i++)
	{
		png_image image = read_png_file(argv[i + 2]);
		png.push_back(image);
	}

	write_ico_file(argv[1], png);

	return 0;
}
