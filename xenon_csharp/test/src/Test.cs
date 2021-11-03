using Xenon;

using System;

namespace TestScripts {

	public class TestScript : Entity {

		public int testValue = 1;
		public float timer = 0;

		public TestScript() {
			Console.WriteLine("Constructor called!");
		}

		public void Start() {
			Console.WriteLine("Start called!");
		}

		public void Update(float delta) {
			Console.WriteLine("Update called!");

			timer += delta;

			Console.WriteLine("Timer: " + timer);

			position += new Vector3(delta, 0, 0);
		}

	}

}
