# Путь к директории сборки
BUILD_DIR = build

# Основные цели
all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)
	@cp $(BUILD_DIR)/motion_detector ./motion_detector || true
	@cp $(BUILD_DIR)/croper/video_cropper ./video_cropper || true

$(BUILD_DIR)/Makefile:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ..

# Очистка
clean:
	rm -rf $(BUILD_DIR)
	rm -f motion_detector video_cropper

# Полная пересборка
rebuild: clean all

.PHONY: all clean rebuild
