import tensorflow as tf
from tensorflow.keras.preprocessing.image import ImageDataGenerator
from tensorflow.keras.applications import MobileNetV2
from tensorflow.keras.layers import Dense, GlobalAveragePooling2D
from tensorflow.keras.models import Model
import os

# --- PATHS ---
DATA_DIR = "../data/dataset"
MODEL_DIR = "../edge_device/assets"
os.makedirs(MODEL_DIR, exist_ok=True)

print("Loading Data...")

# Data augmentation
train_datagen = ImageDataGenerator(
    rescale=1./255,
    rotation_range=20,
    width_shift_range=0.1,
    height_shift_range=0.1,
    horizontal_flip=True, 
    validation_split=0.2
)

train_generator = train_datagen.flow_from_directory(
    DATA_DIR,
    target_size=(224, 224),
    batch_size=32,
    class_mode='categorical',
    subset='training'
)

validation_generator = train_datagen.flow_from_directory(
    DATA_DIR,
    target_size=(224, 224),
    batch_size=32,
    class_mode='categorical',
    subset='validation'
)

# MobileNetV2 model - Transfer Learning
base_model = MobileNetV2(weights='imagenet',
                         include_top=False,
                         input_shape=(224, 224, 3))

base_model.trainable = False # Freeze the pre-trained weights

x = base_model.output
x = GlobalAveragePooling2D()(x)
x = Dense(128, activation="relu")(x)
predictions = Dense(2, activation="softmax")(x)

model = Model(inputs=base_model.input, outputs=predictions)

model.compile(optimizer='adam', 
              loss='categorical_crossentropy', 
              metrics=['accuracy'])

# Train the model
print("STARTING TRAINING...")
model.fit(
    train_generator, 
    epochs=15,
    validation_data=validation_generator
)

# Converting to TFLITE
print("Converting to Tflite...")
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()

# Saving the model
output_path = os.path.join(MODEL_DIR, "drowsiness_model.tflite")
with open(output_path, "wb") as f:
    f.write(tflite_model)

print(f"SUCCESS!! Model saved to : {output_path}")
print(f"Class indices: {train_generator.class_indices}")
