using System.Runtime.InteropServices;
using System;

namespace Xenon {

	[StructLayout(LayoutKind.Sequential)]
	public struct Vector3 {

		public static Vector3 ZERO = new Vector3(0, 0, 0);

		public static Vector3 RIGHT = new Vector3(1, 0, 0);
		public static Vector3 UP = new Vector3(0, 1, 0);
		public static Vector3 FORWARD = new Vector3(0, 0, -1);

		public float x;
		public float y;
		public float z;

		public Vector3(float scalar) {
			x = y = z = scalar;
		}

		public Vector3(float x, float y, float z) {
			this.x = x;
			this.y = y;
			this.z = z;
		}

		public float Length() {
			return (float)Math.Sqrt(x * x + y * y + z * z);
		}

		public Vector3 Normalized() {
			float length = Length();
			float x = this.x / length;
			float y = this.y / length;
			float z = this.z / length;
			return new Vector3(x, y, z);
		}

		public void Normalize() {
			float length = Length();
			x = x / length;
			y = y / length;
			z = z / length;
		}

		public static Vector3 operator *(Vector3 left, float scalar) {
			return new Vector3(left.x * scalar, left.y * scalar, left.z * scalar);
		}

		public static Vector3 operator *(float scalar, Vector3 right) {
			return new Vector3(scalar * right.x, scalar * right.y, scalar * right.z);
		}

		public static Vector3 operator +(Vector3 left, Vector3 right) {
			return new Vector3(left.x + right.x, left.y + right.y, left.z + right.z);
		}

		public static Vector3 operator +(Vector3 left, float right) {
			return new Vector3(left.x + right, left.y + right, left.z + right);
		}

		public static Vector3 operator -(Vector3 left, Vector3 right) {
			return new Vector3(left.x - right.x, left.y - right.y, left.z - right.z);
		}

		public static Vector3 operator /(Vector3 left, Vector3 right) {
			return new Vector3(left.x / right.x, left.y / right.y, left.z / right.z);
		}

		public static Vector3 operator /(Vector3 left, float scalar) {
			return new Vector3(left.x / scalar, left.y / scalar, left.z / scalar);
		}

		public static Vector3 operator -(Vector3 vector) {
			return new Vector3(-vector.x, -vector.y, -vector.z);
		}

		public override string ToString() {
			return "Vector3[" + x + ", " + y + ", " + z + "]";
		}

	}
}
