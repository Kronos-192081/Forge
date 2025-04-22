import streamlit as st
import os
import subprocess

EXECUTABLE="./test_res"
INPUT="tmp/in.txt"
OUTPUT="tmp/out.txt"

def make_result(line):
    res = st.empty()
    res.info(line)

args = (EXECUTABLE, "<", INPUT, ">", OUTPUT)
cmd = " ".join(args)

st.set_page_config(
    page_title="qoptimizer",
    page_icon="🕮",
    layout="wide"
)

st.title('Query Optimizer')
st.header('Compiling queries')

sb = st.sidebar

with sb:
    st.subheader("Outer Rim \nCS39202: Database Management Systems Lab\n2023 Spring")
    st.write("Archit Mangrulkar - 20CS10086")
    st.write("Ashwani Kumar Kamal - 20CS10011")
    st.write("Hardik Pravin Soni - 20CS30023")
    st.write("Shiladitya De - 20CS30061")
    st.write("Sourabh Soumyakanta Das - 20CS30051")

col1, col2 = st.columns(2)

with col1:
    custom_query = st.text_input("Enter Query")

with col2:
    file_upload = st.file_uploader("Upload query file")


compile = st.button("Optimize query")

results = st.empty()

if compile:

    custom_query = custom_query.strip()
    print("custom =", custom_query)

    if file_upload is not None:
        custom_query = file_upload.getvalue().decode("utf-8")
        results.warning("Query uploaded from file.")

    with open(INPUT, "w") as f:
        f.write(custom_query)

    subprocess.run(["python", "call.py"])

    with open(OUTPUT, "r") as f:
        for line in f:
            if line != '\n':
                make_result(line)
