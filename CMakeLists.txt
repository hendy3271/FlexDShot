# FlexPWM DShot Library
# https://github.com/
# MIT License

cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "dshot.cpp"
                       INCLUDE_DIRS ".")

project(FlexDShot)
