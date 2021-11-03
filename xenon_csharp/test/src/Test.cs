using Xenon;

using System;

namespace TestScripts {

	public class TestScript : Entity {

		public int testValue = 1;
		public float timer = 0;
		public float radius = 1;

		public TestScript() {
			Console.WriteLine("Constructor called!");
		}

		public void Start() {
			Console.WriteLine("Start called!");
			PointLightComponent light = CreateComponent<PointLightComponent>();
			light.color = new Vector3(10, 10, 10);
		}

		public void Update(float delta) {
			timer += delta;
			position = new Vector3((float)Math.Sin(timer) * radius, position.y, (float)Math.Cos(timer) * radius);
		}

	}

}
